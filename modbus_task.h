/*
 * modbus_task.h
 *
 *  Created on: 2021-05-05
 *      Author: GDR
 */

#ifndef MODBUS_TASK_H_
#define MODBUS_TASK_H_

extern TaskHandle_t modbus_task_handle;

void modbus_task(void *param);

#endif /* MODBUS_TASK_H_ */
