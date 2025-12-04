
#define _ERRORVALUE         9999 


void sensorTask(void *pvParameters);

typedef struct {
	float voltageH;
    float voltageL;
	int state;
	int mssgCntr;
} sensorMssg_t;

extern sensorMssg_t sensorMssg;


#define UDPSENSORPORT 5001
#define MAXLEN 128
#define SENSOR_TIMEOUT 20 // seconds




