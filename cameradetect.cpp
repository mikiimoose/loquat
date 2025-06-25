#include "cameradetect.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <opencv2/opencv.hpp>

static cv::VideoCapture cap;
static int cam_width = 640;
static int cam_height = 480;

// Finds the /dev/videoX index for a camera with a specific Vendor and Product ID.
// It searches for all matching capture devices and returns the one with the lowest index.
int find_camera_index_by_vid_pid(const char* target_vid, const char* target_pid) {
    const char *v4l_path = "/sys/class/video4linux";
    DIR *dir = opendir(v4l_path);
    if (!dir) {
        perror("Failed to open /sys/class/video4linux");
        return -1;
    }

    int min_index = -1; // To store the lowest valid index found

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "video", 5) != 0) continue;

        char path[PATH_MAX];
        FILE *fp;
        char id[8];
        int found_vid = 0;

        // Check Vendor ID
        snprintf(path, sizeof(path), "%s/%s/device/../idVendor", v4l_path, entry->d_name);
        fp = fopen(path, "r");
        if (!fp) continue;
        if (fgets(id, sizeof(id), fp)) {
            id[strcspn(id, "\n")] = 0;
            if (strcmp(id, target_vid) == 0) {
                found_vid = 1;
            }
        }
        fclose(fp);

        if (!found_vid) continue;

        // If VID matches, check Product ID
        snprintf(path, sizeof(path), "%s/%s/device/../idProduct", v4l_path, entry->d_name);
        fp = fopen(path, "r");
        if (!fp) continue;
        if (fgets(id, sizeof(id), fp)) {
            id[strcspn(id, "\n")] = 0;
            if (strcmp(id, target_pid) == 0) {
                int current_index = atoi(entry->d_name + 5);
                char dev_name[32];
                snprintf(dev_name, sizeof(dev_name), "/dev/video%d", current_index);

                int fd = open(dev_name, O_RDONLY);
                if (fd == -1) {
                    fclose(fp);
                    continue;
                }

                struct v4l2_capability v4l2_cap;
                if (ioctl(fd, VIDIOC_QUERYCAP, &v4l2_cap) == 0) {
                    if (v4l2_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                        printf("Found a valid capture device: %s\n", dev_name);
                        if (min_index == -1 || current_index < min_index) {
                            min_index = current_index;
                        }
                    }
                }
                close(fd);
            }
        }
        fclose(fp);
    }
    closedir(dir);

    if (min_index != -1) {
        printf("Selecting device with lowest index: /dev/video%d\n", min_index);
    }

    return min_index;
}

int camera_init(int width, int height) {
    int camera_index = find_camera_index_by_vid_pid("1224", "2a25");

    if (camera_index < 0) {
        fprintf(stderr, "Could not find a usable capture device with VID:1224 PID:2a25\n");
        return 0;
    }
    
    char device_path[32];
    snprintf(device_path, sizeof(device_path), "/dev/video%d", camera_index);
    printf("Found specific camera at %s, attempting to open directly.\n", device_path);

    cam_width = width;
    cam_height = height;
    
    // Open by full device path to be unambiguous. This is the most reliable method.
    // We explicitly use the V4L2 backend as this is a V4L feature.
    cap.open(device_path, cv::CAP_V4L2);
    
    if (!cap.isOpened()) {
        fprintf(stderr, "Error: Could not open camera via direct path. Check for other apps using it.\n");
        return 0;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, cam_width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, cam_height);
    return 1;
}

int camera_capture(const char *filename) {
    if (!cap.isOpened()) return 0;
    cv::Mat frame;
    // Warm up: grab and discard a few frames
    for (int i = 0; i < 10; ++i) {
        cap >> frame;
        cv::waitKey(30); // wait a bit for the camera to adjust
    }
    if (frame.empty()) return 0;
    double actual_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double actual_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    printf("Camera resolution: %.0fx%.0f\n", actual_width, actual_height);
    return cv::imwrite(filename, frame) ? 1 : 0;
}

void camera_deinit() {
    if (cap.isOpened()) cap.release();
}
