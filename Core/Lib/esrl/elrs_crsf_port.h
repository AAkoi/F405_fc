/**
 * @file    elrs_crsf_port.h
 * @brief   将 ELRS/CRSF 绑定到 UART1 的移植层
 */
#ifndef ELRS_CRSF_PORT_H
#define ELRS_CRSF_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并将 elrs_crsf 绑定到 UART1（默认 420000 波特）
void ELRS_CRSF_InitOnUART1(void);

#ifdef __cplusplus
}
#endif

#endif // ELRS_CRSF_PORT_H

