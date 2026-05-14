#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* hardware drivers    */
#include "uart.h"
#include "lm35.h"
#include "button.h"
#include "motor.h"

/* Motor States */
#define MOTOR_OFF 0
#define MOTOR_ON  1

/* Global Handles */
QueueHandle_t xMotorQueue;
SemaphoreHandle_t xUART_Mutex;

//global variables 
volatile uint8_t overrideActive = 0;

/* -------------------------------------------------------
 * TASK 1: Button Monitoring (Highest Priority)
 * ------------------------------------------------------- */
void vTask_ButtonMonitor(void *pv)
{
    uint8_t motorState = MOTOR_OFF;
    
    for (;;)
    {
		static uint8_t lastButtonState = 1;
		uint8_t currentButtonState = BUTTON_read();
		   if (currentButtonState == 0 && lastButtonState == 1)
		   {
			   overrideActive = 1;
			   motorState = MOTOR_OFF;
			   xQueueSend(xMotorQueue, &motorState, portMAX_DELAY);
			   
			   if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
			   {
				   UART_sendString("[OVERRIDE] Button Open: Forcing Motor OFF\r\n");
				   xSemaphoreGive(xUART_Mutex);
			   }
		   }
		   else if (currentButtonState == 1 && lastButtonState == 0){
			   overrideActive = 0;
		   }
		   lastButtonState = currentButtonState;
        // Small delay to prevent task from starving others, 
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

/* -------------------------------------------------------
 * TASK 2: Temperature Monitoring (Medium Priority)
 * ------------------------------------------------------- */
void vTask_TempMonitor(void *pv)
{
    uint8_t currentTemp;
    uint8_t motorState;

    for (;;)
    {
		if (!overrideActive)
		{
        currentTemp = LM35_read();
        motorState = (currentTemp > TEMP_THRESHOLD) ? MOTOR_ON : MOTOR_OFF;
        xQueueSend(xMotorQueue, &motorState, portMAX_DELAY);

        if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
        {
            UART_sendString("[Sensor] Temp: ");
            uart_print_number(currentTemp);
            UART_sendString(" C\r\n");
            xSemaphoreGive(xUART_Mutex);
        }

       
    }
	 vTaskDelay(pdMS_TO_TICKS(500));
	}
	
}

/* -------------------------------------------------------
 * TASK 3: Motor Execution 
 * Processes states received from the queue.
 * ------------------------------------------------------- */
void vTask_MotorControl(void *pv)
{
    uint8_t receivedState;

    for (;;)
    {

        if (xQueueReceive(xMotorQueue, &receivedState, portMAX_DELAY) == pdTRUE)
        {
            if (receivedState == MOTOR_ON)
            {
                motor_forward();
            }
            else
            {
                motor_stop();
            }

            if (xSemaphoreTake(xUART_Mutex, portMAX_DELAY) == pdTRUE)
            {
                UART_sendString("[Motor] Action: ");
                UART_sendString(receivedState == MOTOR_ON ? "RUNNING\r\n" : "STOPPED\r\n");
                xSemaphoreGive(xUART_Mutex);
            }
        }
    }
}

/* -------------------------------------------------------
 * MAIN
 * ------------------------------------------------------- */
int main(void)
{
    // Initialization
    UART_init(9600);
    LM35_init();
    BUTTON_init();
    motor_init();

    // Create RTOS Objects
    xMotorQueue = xQueueCreate(5, sizeof(uint8_t));
    xUART_Mutex = xSemaphoreCreateMutex();

    if (xMotorQueue != NULL && xUART_Mutex != NULL)
    {
        // Button Monitor: Priority 3 
        xTaskCreate(vTask_ButtonMonitor, "BTN", 100, NULL, 3, NULL);
        
        // Temp Monitor: Priority 2
        xTaskCreate(vTask_TempMonitor, "TEMP", 100, NULL, 2, NULL);
        
        // Motor Control: Priority 1
        xTaskCreate(vTask_MotorControl, "MOT", 100, NULL, 1, NULL);

        vTaskStartScheduler();
    }

    while (1); 
}