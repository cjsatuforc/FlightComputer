/*
 * telemetry.h
 *
 *  Created on: 12/01/2013
 *      Author: alan
 */

#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#define TLM_PRIORITY	1

void StartTelemetry(portTickType interval);
void StopTelemetry(void);
void Telemetry(void * pvParams);

#endif /* TELEMETRY_H_ */
