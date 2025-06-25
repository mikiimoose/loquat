#ifndef CAMERADETECTION_H
#define CAMERADETECTION_H

#ifdef __cplusplus
extern "C" {
#endif

int is_specific_camera_connected();
int camera_init(int width, int height);
void camera_deinit();
int camera_capture(const char *filename);

#ifdef __cplusplus
}
#endif

#endif