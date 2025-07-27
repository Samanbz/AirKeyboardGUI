import argparse
import logging
import os
import sys
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler
from pathlib import Path
import queue
import struct
import time
import threading
import cv2
import numpy as np
import traceback
import mediapipe as mp
from mediapipe.framework.formats import landmark_pb2
import random
import pandas as pd

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('frame_processor.log'),
        logging.StreamHandler()
    ]
)

COLUMNS = [
    'session_frame', 'timestamp', 'hand_index', 'hand_label',
    'hand_score', 'landmark_index', 'x', 'y', 'z',
    'world_x', 'world_y', 'world_z'
]


class HandLandmarker:
    def __init__(self):
        self.hand_landmarker = None

        model_path = "hand_landmarker.task"
        model_path = str((Path(__file__).parent / model_path).resolve())

        BaseOptions = mp.tasks.BaseOptions
        HandLandmarker = mp.tasks.vision.HandLandmarker
        HandLandmarkerOptions = mp.tasks.vision.HandLandmarkerOptions
        VisionRunningMode = mp.tasks.vision.RunningMode

        options = HandLandmarkerOptions(
            base_options=BaseOptions(model_asset_path=model_path),
            running_mode=VisionRunningMode.VIDEO,
            num_hands=2,  # Detect up to 2 hands
            min_hand_detection_confidence=0.6,
            min_hand_presence_confidence=0.5,
            min_tracking_confidence=0.6
        )

        self.last_frame = -1

        self.hand_landmarker = HandLandmarker.create_from_options(options)

    def detect_landmarks(self, frame_number, rgb_frame, relative_timestamp):
        """Detect hand landmarks in the RGB frame"""
        """Detect hand landmarks with retry on timestamp ordering issues"""
        # Convert RGB to MediaPipe format
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        try:
            while frame_number <= self.last_frame:
                logging.warning(
                    f"Frame {frame_number} is not monotonically increasing, spinning.")
                time.sleep(0.003)
                continue

            results = self.hand_landmarker.detect_for_video(
                mp_image, relative_timestamp)
            return results

        except Exception as e:
            logging.error(
                f"Non-timestamp error detecting landmarks: {e}")
            return None

        logging.error(
            f"Failed to detect landmarks after {max_retries} retries")
        return None

    def draw_landmarks(self, results, rgb_frame):
        # Draw landmarks on the frame
        # Loop through the detected hands to visualize

        annotated_image = rgb_frame.copy()

        for idx in range(len(results.hand_landmarks)):
            hand_landmarks = results.hand_landmarks[idx]

            # Draw the hand landmarks
            hand_landmarks_proto = landmark_pb2.NormalizedLandmarkList()
            hand_landmarks_proto.landmark.extend([
                landmark_pb2.NormalizedLandmark(
                    x=landmark.x, y=landmark.y, z=landmark.z)
                for landmark in hand_landmarks
            ])

            mp.solutions.drawing_utils.draw_landmarks(
                annotated_image, hand_landmarks_proto, mp.solutions.hands.HAND_CONNECTIONS,
                mp.solutions.drawing_styles.get_default_hand_landmarks_style(),
                mp.solutions.drawing_styles.get_default_hand_connections_style()
            )

        return annotated_image


class FramePostProcessor:
    def __init__(self, watch_dir, num_workers):
        self.watch_dir = Path(watch_dir)
        self.queue = queue.Queue()
        self.num_workers = num_workers
        self.running = True
        self.start_timestamp = None
        self.landmarks = pd.DataFrame(columns=COLUMNS)

        self.hand_landmarker = HandLandmarker()

    def parse_landmarks(self, results, session_frame, timestamp):
        """
        Parse MediaPipe HandLandmarkerResult into a row for CSV export.
        """
        rows = []

        for h_idx in range(len(results.handedness)):
            for l_idx, landmark in enumerate(results.hand_landmarks[h_idx]):
                row = {
                    'session_frame': session_frame,
                    'timestamp': timestamp,
                    'hand_index': h_idx,
                    'hand_label': 'left' if h_idx == 0 else 'right',
                    'hand_score': results.handedness[h_idx][0].score,
                    'landmark_index': l_idx,
                    'x': landmark.x,
                    'y': landmark.y,
                    'z': landmark.z,
                    'world_x': results.hand_world_landmarks[h_idx][l_idx].x,
                    'world_y': results.hand_world_landmarks[h_idx][l_idx].y,
                    'world_z': results.hand_world_landmarks[h_idx][l_idx].z,
                }
                rows.append(row)

        return pd.DataFrame(rows, columns=COLUMNS)

    def log_landmarks(self, results, frame_path, timestamp):
        # log landmarks to DataFrame
        session_frame = frame_path.stem.split('_')[-1]

        landmarks_df = self.parse_landmarks(
            results, session_frame, timestamp)

        self.landmarks = pd.concat(
            [self.landmarks, landmarks_df], ignore_index=True)

    def save_landmarks(self, output_path):
        if not self.landmarks.empty:
            output_file = Path(output_path).parent / 'landmarks.csv'
            self.landmarks.to_csv(output_file, index=False)
            logging.info(f"Landmarks saved to {output_file}")
        else:
            logging.info("No landmarks to save.")

    def unpack_frame_file(self, frame_path):
        # Read frame header and data
        with open(frame_path, 'rb') as f:
            # Read header: timestamp (8 bytes) + width (4 bytes) + height (4 bytes) data size (4 bytes)
            header_data = f.read(20)
            if len(header_data) < 20:
                logging.error(f"Invalid file: {frame_path}")
                return None, None, None, None

            timestamp, width, height, data_size = struct.unpack(
                '<QIII', header_data)

            frame_data = f.read(data_size)

            # check if framepath ends with 00000 without suffix
            if frame_path.stem.endswith('_000000') and self.start_timestamp is None:
                self.start_timestamp = timestamp

            if len(frame_data) != data_size:
                logging.error(
                    f"Incomplete frame data: {frame_path}, expected {data_size} bytes, got {len(frame_data)} bytes.")
                return None, None, None, None

            return frame_data, timestamp, width, height

    def process_frame(self, frame_path):
        frame_path = Path(frame_path)
        logging.info(f"Processing frame: {frame_path.name}")

        try:
            rgb_frame, timestamp, width, height = self.unpack_frame_file(
                frame_path)
            rgb_frame = np.frombuffer(
                rgb_frame, dtype=np.uint8).reshape((height, width, 3))

            if rgb_frame is None or timestamp is None:
                logging.error(f"Failed to unpack frame: {frame_path}")
                return

            # If first frame hasn't been processed yet, wait for it
            while self.start_timestamp is None:
                time.sleep(0.033)

            # Detect landmarks
            relative_timestamp = timestamp - self.start_timestamp
            frame_number = int(frame_path.stem.split('_')[-1])
            results = self.hand_landmarker.detect_landmarks(
                frame_number, rgb_frame, relative_timestamp)
            if not results or not results.hand_landmarks:
                logging.warning(
                    f"No landmarks detected for frame {frame_path.name}, skipping.")

            if results and results.hand_landmarks:
                self.log_landmarks(results, frame_path, timestamp)

                rgb_frame = self.hand_landmarker.draw_landmarks(
                    results, rgb_frame)

            # Quality 95 preserves hand details well
            cv2.imwrite(frame_path.with_suffix(".jpg"), rgb_frame, [
                cv2.IMWRITE_JPEG_QUALITY, 95])

            # Delete original raw file
            os.remove(frame_path)

        except Exception as e:
            logging.error(
                f"Error processing frame {frame_path.name}: {e}")
            logging.error(traceback.format_exc())
            return

    def worker_thread(self):
        while self.running:
            try:
                frame_path = self.queue.get(timeout=1)
                if frame_path is None:
                    break
                self.process_frame(frame_path)
                self.queue.task_done()
            except queue.Empty:
                continue
            except Exception as e:
                logging.error(f"Worker thread error: {e}")

    def start_workers(self):
        self.threads = []
        for i in range(self.num_workers):
            thread = threading.Thread(
                target=self.worker_thread, name=f"Worker-{i+1}")
            thread.daemon = True
            thread.start()
            self.threads.append(thread)
        logging.info(f"Started {self.num_workers} worker threads")

    def stop_workers(self):
        self.running = False

        for _ in range(self.num_workers):
            self.queue.put(None)

        for thread in self.threads:
            thread.join()

        logging.info("All worker threads have been stopped.")

    def process_existing_frames(self):
        raw_files = list(self.watch_dir.glob("*.raw"))
        for frame_file in raw_files:
            self.queue.put(str(frame_file))
        logging.info(f"Queued {len(raw_files)} existing frames")


class FrameHandler(FileSystemEventHandler):
    def __init__(self, converter):
        self.converter = converter

    def on_created(self, event):
        if not event.is_directory and event.src_path.endswith('.raw'):
            time.sleep(0.01)
            self.converter.queue.put(event.src_path)


def main():
    parser = argparse.ArgumentParser(
        description='Convert raw NV12 frames to PNG format')
    parser.add_argument('watch_dir', help='Directory to watch for frame files')
    parser.add_argument('--workers', type=int, default=4,
                        help='Number of worker threads')
    args = parser.parse_args()

    logging.info(f"Starting frame converter with {args.workers} workers.")

    if not os.path.exists(args.watch_dir):
        logging.error(f"Watch directory {args.watch_dir} does not exist.")
        sys.exit(1)

    if not os.path.isdir(args.watch_dir):
        logging.error(f"Watch directory {args.watch_dir} is not a directory.")
        sys.exit(1)

    logging.info(f"Watching directory: {args.watch_dir}")

    converter = FramePostProcessor(args.watch_dir, num_workers=args.workers)

    # Process any existing frames
    converter.process_existing_frames()

    # Start worker threads
    converter.start_workers()

    # Set up file watcher
    event_handler = FrameHandler(converter)
    observer = Observer()
    observer.schedule(event_handler, args.watch_dir, recursive=False)
    observer.start()

    shutdown_signal_path = Path(args.watch_dir) / '.shutdown'

    # Remove any existing shutdown signal
    if shutdown_signal_path.exists():
        shutdown_signal_path.unlink()

    try:
        while True:
            # Check for shutdown signal
            if shutdown_signal_path.exists():
                logging.info("Shutdown signal detected, finishing queue...")
                break

            time.sleep(1)
            if converter.queue.qsize() > 0:
                logging.info(f"Queue size: {converter.queue.qsize()}")

    except KeyboardInterrupt:
        logging.info("\nShutting down...")

    # Graceful shutdown
    observer.stop()
    observer.join()

    # Wait for queue to empty
    logging.info(
        f"Waiting for {converter.queue.qsize()} frames to finish processing...")
    converter.queue.join()  # Block until all tasks are done

    # Then stop workers
    converter.stop_workers()
    converter.save_landmarks(args.watch_dir)

    # Clean up shutdown signal
    if shutdown_signal_path.exists():
        shutdown_signal_path.unlink()

    logging.info("Shutdown complete")


if __name__ == "__main__":
    try:
        main()
        # Execute command in watch dir
        watch_dir = Path(sys.argv[1])
        if not watch_dir.exists():
            logging.error(f"Watch directory {watch_dir} does not exist.")
            sys.exit(1)

        if not watch_dir.is_dir():
            logging.error(f"Watch directory {watch_dir} is not a directory.")
            sys.exit(1)

        os.chdir(watch_dir)

        os.system(
            'ffmpeg -i frame_%06d.jpg -c:v libx264 -pix_fmt yuv420p -r 30 ../output.mp4')

        os.chdir(watch_dir.parent)
        # Remove the watch directory after processing even if its full
        if watch_dir.exists():
            for item in watch_dir.iterdir():
                if item.is_file():
                    item.unlink()
                elif item.is_dir():
                    os.rmdir(item)
            watch_dir.rmdir()
            logging.info(f"Removed watch directory: {watch_dir}")

    except KeyboardInterrupt:
        logging.info("Stopping frame converter due to keyboard interrupt.")
        sys.exit(0)
    except Exception as e:
        logging.error(f"An error occurred: {e}")
        logging.error(traceback.format_exc())
        sys.exit(1)
