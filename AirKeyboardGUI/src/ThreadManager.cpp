#include "ThreadManager.h"

void ThreadManager::subscribeToEvents() {
    EventBus::getInstance().subscribe(AppEvent::START_LOGGING, [this]() {
        if (!logging) {
            logging = true;
            startLogging();
        }
    });

    EventBus::getInstance().subscribe(AppEvent::STOP_LOGGING, [this]() {
        if (logging) {
            logging = false;
            stopLogging();
        }
    });

    EventBus::getInstance().subscribe(AppEvent::TOGGLE_LOGGING, [this]() {
        logging = !logging;
        if (logging) {
            startLogging();
        } else {
            stopLogging();
        }
    });
}

void ThreadManager::startCapturing() {
    framePublisherThread = std::thread([this]() {
        FramePublisher framePublisher{};
        const auto interval = std::chrono::milliseconds(33);
        auto next_time = std::chrono::steady_clock::now();
        while (running) {
            framePublisher.captureFrame();
            next_time += interval;
            std::this_thread::sleep_until(next_time);
        }
    });

    frameProcessorThread = std::thread([this]() {
        FramePublisher* framePublisher = FramePublisher::getInstance();
        FrameProcessor& frameProcessor = FrameProcessor::getInstance();

        framePublisher->subscribe(&frameProcessor);

        while (running) {
            frameProcessor.dequeue();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        framePublisher->unsubscribe(&frameProcessor);
    });

    keyEventPublisherThread = std::thread([this]() {
        KeyEventPublisher& keyEventPublisher = KeyEventPublisher::getInstance();
        keyEventPublisherReady.set_value();  // Notify that publisher is ready

        MSG msg;
        while (running) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
    });
}

void ThreadManager::startLogging() {
    // Identifier for the current logging session based on current time
    std::string logSessionId = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    std::filesystem::path baseUrl = std::filesystem::current_path() / LOG_DIR / logSessionId;

    // Create the base directory for logging
    std::filesystem::create_directories(baseUrl);

    static char g_debugBuffer[256];
    sprintf(g_debugBuffer, "AirKeyboardGUI: Starting logging session at %s\n", baseUrl.string().c_str());
    OutputDebugStringA(g_debugBuffer);

    keyLoggerThread = std::thread([this, baseUrl]() {
        std::filesystem::path logFilePath = baseUrl / "key_events.csv";

        KeyEventLogger keyEventLogger{logFilePath};
        KeyEventPublisher& keyEventPublisher = KeyEventPublisher::getInstance();
        keyEventPublisher.subscribe(&keyEventLogger);

        while (logging) {
            // Wait for batch size or timeout
            if (keyEventLogger.waitForBatch(std::chrono::milliseconds(500))) {
                // We hit batch size - flush immediately
                keyEventLogger.flush();
            } else {
                // Timeout - flush any pending messages
                keyEventLogger.flush();
            }
        }

        keyEventPublisher.unsubscribe(&keyEventLogger);
    });

    frameLoggerThread = std::thread([this, baseUrl]() {
        std::filesystem::path logDir = baseUrl / "frames";
        std::filesystem::create_directories(logDir);

        FrameLogger frameLogger{logDir};

        FramePostProcessor framePostProcessor{logDir.string()};
        framePostProcessor.SpawnWorker();

        FrameProcessor& frameProcessor = FrameProcessor::getInstance();

        frameProcessor.subscribe(&frameLogger);
        while (logging) {
            // Wait for batch size or timeout
            if (frameLogger.waitForBatch(std::chrono::milliseconds(500))) {
                // We hit batch size - flush immediately
                frameLogger.flush();
            } else {
                // Timeout - flush any pending messages
                frameLogger.flush();
            }
        }

        frameProcessor.unsubscribe(&frameLogger);

        framePostProcessor.terminateWorker();
    });
}

void ThreadManager::stopLogging() {
    if (keyLoggerThread.joinable()) {
        keyLoggerThread.join();
    }

    if (frameLoggerThread.joinable()) {
        frameLoggerThread.join();
    }
}

ThreadManager::ThreadManager() {
    subscribeToEvents();
}

void ThreadManager::start() {
    running = true;
    keyEventPublisherFuture = keyEventPublisherReady.get_future();

    startCapturing();

    textUiThread = std::thread([this]() {
        TextContainer textContainer;
        keyEventPublisherFuture.wait();

        KeyEventPublisher& keyPublisher = KeyEventPublisher::getInstance();
        keyPublisher.subscribe(&textContainer);

        while (running) {
            textContainer.dequeue();

            MSG msg;
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                // If no window messages, just sleep for a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        keyPublisher.unsubscribe(&textContainer);
    });

    liveKeyboardViewThread = std::thread([this]() {
        RECT rootWindowRect;
        GetClientRect(g_hMainWindow, &rootWindowRect);

        int rootWidth = rootWindowRect.right - rootWindowRect.left;
        int rootHeight = rootWindowRect.bottom - rootWindowRect.top;

        int viewWidth = rootWidth / 3;
        int viewHeight = viewWidth * 9 / 16;

        LiveKeyboardView liveKeyboardView{};
        FrameProcessor& frameProcessor = FrameProcessor::getInstance();
        frameProcessor.subscribe(&liveKeyboardView);
        const auto interval = std::chrono::milliseconds(33);
        auto next_time = std::chrono::steady_clock::now();

        while (running) {
            liveKeyboardView.dequeue();
            MSG msg;
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            std::this_thread::sleep_until(next_time);
        }

        frameProcessor.unsubscribe(&liveKeyboardView);
    });

    loggingTriggerThread = std::thread([this]() {
        LoggingTrigger& logTrigger = LoggingTrigger::getInstance();
        keyEventPublisherFuture.wait();

        KeyEventPublisher& keyEventPublisher = KeyEventPublisher::getInstance();
        keyEventPublisher.subscribe(&logTrigger);
        while (running) {
            logTrigger.dequeue();
            MSG msg;
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            logTrigger.checkAutoStop();
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
        keyEventPublisher.unsubscribe(&logTrigger);
    });
}

void ThreadManager::stop() {
    running = false;

    if (framePublisherThread.joinable()) {
        framePublisherThread.join();
    }
    if (frameProcessorThread.joinable()) {
        frameProcessorThread.join();
    }
    if (keyEventPublisherThread.joinable()) {
        keyEventPublisherThread.join();
    }
    if (textUiThread.joinable()) {
        textUiThread.join();
    }
    if (liveKeyboardViewThread.joinable()) {
        liveKeyboardViewThread.join();
    }
    if (loggingTriggerThread.joinable()) {
        loggingTriggerThread.join();
    }
}