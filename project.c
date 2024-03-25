#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_FRAMES 3
#define NUM_PAGES 10

typedef struct {
    int pageId;
    int referenced;
} PageFrame;

typedef struct {
    PageFrame frames[NUM_FRAMES];
    int currentIndex;
    int pageHitCount; // New addition: count of page hits
    int totalAccessCount; // New addition: count of total page accesses
    pthread_mutex_t mutex;
} CircularQueue;

void initializeQueue(CircularQueue* queue) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        queue->frames[i].pageId = -1;
        queue->frames[i].referenced = 0;
    }
    queue->currentIndex = 0;
    queue->pageHitCount = 0; // Initialize page hit count
    queue->totalAccessCount = 0; // Initialize total access count
    pthread_mutex_init(&queue->mutex, NULL);
}

void displayQueue(CircularQueue* queue) {
    pthread_mutex_lock(&queue->mutex);
    printf("Page Frames: ");
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (queue->frames[i].pageId == -1) {
            printf("[ ] ");
        } else {
            printf("[%d] ", queue->frames[i].pageId);
        }
    }
    printf("\n");
    printf("Page Hit Ratio: %.2f%%\n", (float)(queue->pageHitCount) / queue->totalAccessCount * 100);
    pthread_mutex_unlock(&queue->mutex);
}

void replacePage(CircularQueue* queue, int newPageId) {
    while (1) {
        pthread_mutex_lock(&queue->mutex);
        if (queue->frames[queue->currentIndex].referenced == 1) {
            queue->frames[queue->currentIndex].referenced = 0;
            queue->currentIndex = (queue->currentIndex + 1) % NUM_FRAMES;
        } else {
            // New page added
            if (queue->frames[queue->currentIndex].pageId != -1) {
                queue->pageHitCount++; // Increment page hit count
            }
            queue->frames[queue->currentIndex].pageId = newPageId;
            queue->frames[queue->currentIndex].referenced = 0;
            queue->currentIndex = (queue->currentIndex + 1) % NUM_FRAMES;
            pthread_mutex_unlock(&queue->mutex);
            break;
        }
        pthread_mutex_unlock(&queue->mutex);
    }
    queue->totalAccessCount++; // Increment total access count
}

void* pageAccessTask(void* arg) {
    CircularQueue* pageQueue = (CircularQueue*)arg;
    int referenceString[] = {0, 1, 2, 3, 0, 1, 4, 0, 1, 2, 3, 4};
    int referenceSize = sizeof(referenceString) / sizeof(referenceString[0]);
    for (int i = 0; i < referenceSize; i++) {
        int currentPage = referenceString[i];
        int pageFound = 0;
        pthread_mutex_lock(&pageQueue->mutex);
        for (int j = 0; j < NUM_FRAMES; j++) {
            if (pageQueue->frames[j].pageId == currentPage) {
                pageQueue->frames[j].referenced = 1;
                pageFound = 1;
                break;
            }
        }
        pthread_mutex_unlock(&pageQueue->mutex);
        if (!pageFound) {
            replacePage(pageQueue, currentPage);
        }
        displayQueue(pageQueue);
        usleep(500000); // Introduce a delay to simulate page access time
    }
    return NULL;
}

int main() {
    CircularQueue pageQueue;
    initializeQueue(&pageQueue);
    pthread_t thread;
    pthread_create(&thread, NULL, pageAccessTask, (void*)&pageQueue);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&pageQueue.mutex);
    return 0;
}
