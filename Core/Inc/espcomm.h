#ifndef APPS_ESPCOMM_ESPCOMM_H_
#define APPS_ESPCOMM_ESPCOMM_H_




void EspComm_init();
void debugPrint(const char *fmt, ...);
void SyncTime();
void info();
void EspCmdHandler();
//void mqtt_data_send(char* data);
//void mqtt_debug_send(char* data);
//void mqtt_saved_data_send(char* data);
//void UART5_SetBaud(uint32_t baud);
#endif /* APPS_ESPCOMM_ESPCOMM_H_ */
