#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

int snapshot_take(void);
int snapshot_thread_initialize(void);
void snapshot_thread_deinitialize(void);
void trigger_snapshot(void);

#ifdef __cplusplus
}
#endif

#endif 