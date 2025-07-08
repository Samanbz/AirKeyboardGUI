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

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('frame_processor.log'),
        logging.StreamHandler()
    ]
)


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

        self.hand_landmarker = HandLandmarker.create_from_options(options)

    def detect_landmarks(self, rgb_frame, relative_timestamp, max_retries=10):
        """Detect hand landmarks in the RGB frame"""
        """Detect hand landmarks with retry on timestamp ordering issues"""
        for attempt in range(max_retries):
            try:
                mp_image = mp.Image(
                    image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                results = self.hand_landmarker.detect_for_video(
                    mp_image, relative_timestamp)
                return results

            except Exception as e:
                error_msg = str(e).lower()
                # Check if it's a timestamp ordering issue
                if "must be monotonically increasing" in error_msg:
                    # Wait a bit and retry
                    sleep_time = 0.001 + \
                        random.uniform(0, 0.001)  # 1ms + jitter
                    logging.warning(
                        f"Timestamp ordering issue, retry {attempt+1}/{max_retries} after {sleep_time:.5f}s")
                    time.sleep(sleep_time)
                    continue
                else:
                    # Different error, don't retry
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

        self.hand_landmarker = HandLandmarker()

    def nv12_to_rgb(self, nv12_data, width, height):
        """Convert NV12 format to RGB"""
        yuv = np.frombuffer(nv12_data, dtype=np.uint8)

        # Y plane
        y = yuv[:width*height].reshape((height, width))

        # UV plane (interleaved, half resolution)
        uv_start = width * height
        uv = yuv[uv_start:uv_start +
                 (width*height//2)].reshape((height//2, width//2, 2))

        # Upsample UV to full resolution
        u = cv2.resize(uv[:, :, 0], (width, height),
                       interpolation=cv2.INTER_LINEAR)
        v = cv2.resize(uv[:, :, 1], (width, height),
                       interpolation=cv2.INTER_LINEAR)

        # Convert YUV to RGB
        yuv_img = cv2.merge([y, u, v])
        rgb = cv2.cvtColor(yuv_img, cv2.COLOR_YUV2RGB)
        return rgb

    def unpack_frame_file(self, frame_path):
        # Read frame header and data
        with open(frame_path, 'rb') as f:
            # Read header: timestamp (8 bytes) + data size (4 bytes)
            header_data = f.read(12)
            if len(header_data) < 12:
                logging.error(f"Invalid file: {frame_path}")
                return None, None

            timestamp, data_size = struct.unpack('<QI', header_data)
            # check if framepath ends with 00000 without suffix
            if frame_path.stem.endswith('00000') and self.start_timestamp is None:
                self.start_timestamp = timestamp

            frame_data = f.read(data_size)

            if len(frame_data) != data_size:
                logging.error(
                    f"Incomplete frame data: {frame_path}, expected {data_size} bytes, got {len(frame_data)} bytes.")
                return None, None

            return frame_data, timestamp

    def save_landmarks(self, results, frame_path, timestamp):

        landmarks = results.hand_landmarks
        if not landmarks:
            logging.warning(f"No landmarks found for frame {frame_path}")
            return

        # Create landmarks directory if it doesn't exist
        landmarks_dir = self.watch_dir / 'landmarks'
        landmarks_dir.mkdir(exist_ok=True)

        # Save timestamp
        timestamp_str = f"{timestamp:013d}"
        landmarks_file = landmarks_dir / \
            f"{frame_path.stem}_{timestamp_str}.txt"
        with open(landmarks_file, 'w') as f:
            for idx, hand_landmarks in enumerate(landmarks):
                f.write(f"Hand {idx + 1}:\n")
                for landmark in hand_landmarks:
                    f.write(
                        f"{landmark.x:.6f} {landmark.y:.6f} {landmark.z:.6f}\n")
                f.write("\n")

        logging.info(f"Saved landmarks to {landmarks_file}")

    def process_frame(self, frame_path):
        logging.info(f"Processing frame: {frame_path}")
        frame_path = Path(frame_path)

        try:
            frame_data, timestamp = self.unpack_frame_file(frame_path)

            if frame_data is None or timestamp is None:
                logging.error(f"Failed to unpack frame: {frame_path}")
                return

            # Convert NV12 to RGB
            rgb_frame = self.nv12_to_rgb(frame_data, 1920, 1080)

            # Rotate if needed (keeping your rotation)
            rgb_frame = cv2.rotate(rgb_frame, cv2.ROTATE_180)

            # If first frame hasn't been processed yet, wait for it
            while self.start_timestamp is None:
                time.sleep(0.033)

            # Detect landmarks
            relative_timestamp = timestamp - self.start_timestamp
            results = self.hand_landmarker.detect_landmarks(
                rgb_frame, relative_timestamp)
            if not results or not results.hand_landmarks:
                logging.warning(
                    f"No landmarks detected for frame {frame_path}, skipping.")

            if results and results.hand_landmarks:
                self.save_landmarks(results, frame_path, timestamp)

                rgb_frame = self.hand_landmarker.draw_landmarks(
                    results, rgb_frame)

                # Convert RGB to BGR for OpenCV
            bgr_frame = cv2.cvtColor(rgb_frame, cv2.COLOR_RGB2BGR)

            # Quality 95 preserves hand details well
            cv2.imwrite(frame_path.with_suffix(".jpg"), bgr_frame, [
                        cv2.IMWRITE_JPEG_QUALITY, 95])

            # Delete original raw file
            os.remove(frame_path)

        except Exception as e:
            logging.error(f"Error processing frame {frame_path}: {e}")
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
            time.sleep(0.05)  # Small delay to ensure file is fully written
            self.converter.queue.put(event.src_path)


def main():
    import argparse

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
                time.sleep(3)
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
            'ffmpeg -i frame_%06d.jpg -c:v libx264 -pix_fmt yuv420p -r 30 output.mp4')
    except KeyboardInterrupt:
        logging.info("Stopping frame converter due to keyboard interrupt.")
        sys.exit(0)
    except Exception as e:
        logging.error(f"An error occurred: {e}")
        logging.error(traceback.format_exc())
        sys.exit(1)
