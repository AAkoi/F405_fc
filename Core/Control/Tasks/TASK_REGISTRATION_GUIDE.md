# 飞控任务注册方式详解

## 📚 三种注册方式对比

调度器提供了三种任务注册方式，分别适用于不同的场景：

### 1️⃣ `scheduler_register_periodic` - 周期性任务

**特点：基于时间，固定周期执行**

```c
bool scheduler_register_periodic(
    task_scheduler_fc_t *sched,
    const char *name,           // 任务名称
    task_cb_t callback,         // 回调函数
    void *user_data,            // 用户数据
    task_priority_t priority,   // 优先级
    uint32_t period_us,         // 周期（微秒）
    uint32_t max_exec_us        // 最大执行时间（0=不限制）
);
```

**执行条件：** `当前时间 >= 下次运行时间`

**适用场景：**
- ✅ 需要固定频率执行的任务
- ✅ 不依赖外部事件的任务
- ✅ 控制回路、传感器轮询等

---

### 2️⃣ `scheduler_register_event_flag` - 标志位触发任务

**特点：基于外部标志位，标志位置1时执行**

```c
bool scheduler_register_event_flag(
    task_scheduler_fc_t *sched,
    const char *name,
    task_cb_t callback,
    void *user_data,
    task_priority_t priority,
    volatile bool *event_flag,  // 指向标志位的指针
    uint32_t max_exec_us
);
```

**执行条件：** `*event_flag == true`  
**执行后：** 调度器自动将标志位清零

**适用场景：**
- ✅ 由中断设置标志位的任务
- ✅ 数据就绪信号（IMU、DMA完成等）
- ✅ 简单的事件响应

---

### 3️⃣ `scheduler_register_event_callback` - 回调判断触发任务

**特点：通过回调函数动态判断是否应该执行**

```c
bool scheduler_register_event_callback(
    task_scheduler_fc_t *sched,
    const char *name,
    task_cb_t callback,
    task_should_run_cb_t should_run,  // 判断函数
    void *user_data,
    task_priority_t priority,
    uint32_t max_exec_us
);
```

**执行条件：** `should_run(user_data)` 返回 `true`  
**执行后：** 不会自动清除任何标志位（需要在任务中手动处理）

**适用场景：**
- ✅ 复杂的触发条件（多个标志位组合）
- ✅ 需要读取状态机、缓冲区等判断
- ✅ 条件不是简单的标志位

---

## 🎯 实际应用示例

### 示例 1：周期性任务 - PID 控制器

**场景：** PID 控制器需要固定 1KHz 运行

```c
// 任务回调函数
void task_pid_control(void *user)
{
    // 读取当前姿态
    float roll = Attitude_GetRoll();
    float pitch = Attitude_GetPitch();
    float yaw = Attitude_GetYaw();
    
    // 获取遥控器目标值
    float target_roll = rc_get_roll();
    float target_pitch = rc_get_pitch();
    float target_yaw_rate = rc_get_yaw_rate();
    
    // 计算误差并更新 PID
    float roll_output = PID_Update(&pid_roll, target_roll - roll, 0.001f);
    float pitch_output = PID_Update(&pid_pitch, target_pitch - pitch, 0.001f);
    float yaw_output = PID_Update(&pid_yaw, target_yaw_rate - yaw, 0.001f);
    
    // 输出到电机混控器
    Motor_Mix(roll_output, pitch_output, yaw_output);
}

// 注册任务
void register_pid_task(task_scheduler_fc_t *sched)
{
    scheduler_register_periodic(
        sched,
        "PID_Control",          // 任务名
        task_pid_control,       // 回调函数
        NULL,                   // 无用户数据
        TASK_PRIORITY_HIGH,     // 高优先级
        1000,                   // 1000us = 1KHz
        300                     // 最大允许 300us
    );
}
```

**为什么选择周期性？**
- PID 控制器需要固定频率
- 不依赖外部事件
- 稳定的采样周期保证控制效果

---

### 示例 2：标志位事件任务 - IMU 数据处理

**场景：** IMU 数据就绪中断触发处理

```c
// 全局标志位（在中断中设置）
extern volatile uint8_t icm42688p_data_ready;

// 中断服务程序（在 icm42688p.c 中）
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ICM42688P_INT_PIN) {
        icm42688p_data_ready = 1;  // 设置标志位
    }
}

// 任务回调函数
void task_imu_process(void *user)
{
    int16_t gx, gy, gz, ax, ay, az;
    float temp;
    
    // 读取 IMU 数据
    if (icm42688p_update(&gx, &gy, &gz, &ax, &ay, &az, &temp)) {
        // 喂给滤波器
        gyro_filter_feed_sample(gx, gy, gz);
    }
}

// 注册任务
void register_imu_task(task_scheduler_fc_t *sched)
{
    scheduler_register_event_flag(
        sched,
        "IMU_Process",
        task_imu_process,
        NULL,
        TASK_PRIORITY_CRITICAL,
        (volatile bool *)&icm42688p_data_ready,  // 标志位指针
        50                                        // 最大 50us（8KHz 周期是 125us）
    );
}
```

**为什么选择标志位事件？**
- IMU 中断触发，不是固定周期
- 标志位简单明确
- 调度器自动清零标志位，代码简洁

---

### 示例 3：回调判断任务 - 串口数据处理

**场景：** 只有当串口接收缓冲区有数据时才处理

```c
#include "bsp_uart.h"

// 判断函数：检查是否有数据可读
bool should_run_uart_task(void *user)
{
    // 检查 UART 接收缓冲区是否有数据
    return (uart_get_rx_count() > 0);
}

// 任务回调函数
void task_uart_process(void *user)
{
    uint8_t buffer[256];
    uint16_t len = uart_read_data(buffer, sizeof(buffer));
    
    if (len > 0) {
        // 解析 ELRS CRSF 协议
        elrs_parse_packet(buffer, len);
    }
}

// 注册任务
void register_uart_task(task_scheduler_fc_t *sched)
{
    scheduler_register_event_callback(
        sched,
        "UART_Process",
        task_uart_process,
        should_run_uart_task,  // 判断函数
        NULL,
        TASK_PRIORITY_NORMAL,
        200
    );
}
```

**为什么选择回调判断？**
- 需要检查缓冲区状态
- 不是简单的标志位（可能有多个字节的判断）
- 更灵活的触发条件

---

### 示例 4：复杂条件的回调判断 - 姿态更新

**场景：** 同时满足多个条件时才执行

```c
extern gyro_decim_t gyro_decim;
extern volatile bool accel_ready;
extern bool mag_calibrated;

// 判断函数：复杂条件组合
bool should_run_attitude_update(void *user)
{
    // 必须同时满足：
    // 1. 陀螺仪降采样数据就绪
    // 2. 加速度计数据就绪
    // 3. 磁力计已校准（如果需要航向融合）
    
    return (gyro_decim.ready && accel_ready && mag_calibrated);
}

// 任务回调函数
void task_attitude_update(void *user)
{
    int16_t ax, ay, az;
    icm42688p_get_accel_data(&ax, &ay, &az);
    
    // 更新姿态融合
    Attitude_Update(
        gyro_decim.dps_x, 
        gyro_decim.dps_y, 
        gyro_decim.dps_z,
        ax, ay, az
    );
    
    // 手动清除标志位
    gyro_decim.ready = false;
    accel_ready = false;
}

// 注册任务
void register_attitude_task(task_scheduler_fc_t *sched)
{
    scheduler_register_event_callback(
        sched,
        "Attitude_Update",
        task_attitude_update,
        should_run_attitude_update,  // 复杂判断
        NULL,
        TASK_PRIORITY_CRITICAL,
        200
    );
}
```

**为什么选择回调判断？**
- 需要同时满足多个条件
- 不是单一标志位
- 需要手动控制标志位清除顺序

---

### 示例 5：带用户数据的任务 - 多传感器管理

**场景：** 使用同一个回调函数处理多个传感器

```c
typedef struct {
    uint8_t sensor_id;
    uint32_t last_read_time;
    volatile bool *data_ready_flag;
} sensor_context_t;

// 传感器上下文
sensor_context_t baro_ctx = {
    .sensor_id = 0,
    .last_read_time = 0,
    .data_ready_flag = &baro_ready_flag
};

sensor_context_t mag_ctx = {
    .sensor_id = 1,
    .last_read_time = 0,
    .data_ready_flag = &mag_ready_flag
};

// 通用传感器处理函数
void task_sensor_read(void *user)
{
    sensor_context_t *ctx = (sensor_context_t *)user;
    
    switch (ctx->sensor_id) {
        case 0:  // 气压计
            bmp280_read_data();
            break;
        case 1:  // 磁力计
            hmc5883l_read_data();
            break;
    }
    
    ctx->last_read_time = HAL_GetTick();
}

// 注册多个传感器任务
void register_sensor_tasks(task_scheduler_fc_t *sched)
{
    // 气压计 - 50Hz
    scheduler_register_periodic(
        sched,
        "Baro_Read",
        task_sensor_read,
        &baro_ctx,              // 传递气压计上下文
        TASK_PRIORITY_LOW,
        20000,
        1000
    );
    
    // 磁力计 - 100Hz
    scheduler_register_periodic(
        sched,
        "Mag_Read",
        task_sensor_read,
        &mag_ctx,               // 传递磁力计上下文
        TASK_PRIORITY_LOW,
        10000,
        500
    );
}
```

**为什么使用用户数据？**
- 复用同一个回调函数
- 通过上下文区分不同的传感器
- 减少代码重复

---

## 📊 三种方式对比表

| 特性 | 周期性任务 | 标志位事件任务 | 回调判断任务 |
|------|-----------|---------------|-------------|
| **触发方式** | 固定时间周期 | 外部标志位置1 | 回调函数返回true |
| **标志清除** | 不需要 | 自动清零 | 需要手动清除 |
| **中断友好** | ❌ | ✅ | ✅ |
| **复杂条件** | ❌ | ❌ | ✅ |
| **代码简洁** | ✅ | ✅ | ⚠️（需要额外函数） |
| **实时性** | 中 | 高 | 高 |
| **CPU占用** | 低 | 低 | 中（需要频繁调用判断函数） |

---

## 🎓 选择指南

### 选择 **周期性任务** 如果：
- ✅ 任务需要固定频率执行
- ✅ 例如：PID控制、LED闪烁、定时发送数据

### 选择 **标志位事件任务** 如果：
- ✅ 由中断设置简单标志位
- ✅ 单一触发条件
- ✅ 例如：IMU数据就绪、DMA完成、按键中断

### 选择 **回调判断任务** 如果：
- ✅ 需要检查多个条件
- ✅ 需要读取硬件状态寄存器
- ✅ 复杂的触发逻辑
- ✅ 例如：串口缓冲区有数据、状态机特定状态、多传感器同步

---

## 🚀 完整示例：混合使用

```c
void fc_tasks_init(task_scheduler_fc_t *sched)
{
    // 1. 周期性任务 - PID 控制 (1KHz)
    scheduler_register_periodic(
        sched, "PID_Control", task_pid_control,
        NULL, TASK_PRIORITY_HIGH, 1000, 300
    );
    
    // 2. 标志位事件 - IMU 中断触发 (最高优先级)
    scheduler_register_event_flag(
        sched, "IMU_Process", task_imu_process,
        NULL, TASK_PRIORITY_CRITICAL,
        (volatile bool *)&imu_data_ready, 50
    );
    
    // 3. 回调判断 - 串口数据处理（有数据才处理）
    scheduler_register_event_callback(
        sched, "UART_Process", task_uart_process,
        should_run_uart_task, NULL,
        TASK_PRIORITY_NORMAL, 200
    );
    
    // 4. 周期性任务 - LED 状态 (10Hz)
    scheduler_register_periodic(
        sched, "LED_Update", task_led_update,
        NULL, TASK_PRIORITY_IDLE, 100000, 100
    );
}
```

---

## ⚠️ 注意事项

### 标志位事件任务：
```c
// ❌ 错误：在任务中手动清零标志位（调度器会重复清零）
void task_imu_process(void *user) {
    // 处理数据...
    icm42688p_data_ready = 0;  // 不需要！调度器会自动清零
}

// ✅ 正确：让调度器自动清零
void task_imu_process(void *user) {
    // 处理数据...
    // 无需清零，调度器会处理
}
```

### 回调判断任务：
```c
// ❌ 错误：忘记清除标志位
void task_uart_process(void *user) {
    uart_read_data(buffer, len);
    // 忘记清除！下次会立即再次执行
}

// ✅ 正确：手动清除标志位
void task_uart_process(void *user) {
    uart_read_data(buffer, len);
    // 必须手动清除缓冲区或标志位
}
```

### 周期性任务：
```c
// ⚠️ 注意：避免任务执行时间超过周期
void task_pid_control(void *user) {
    // ❌ 如果这里执行超过 1000us，会导致错过周期
    HAL_Delay(2);  // 2ms - 太长了！
    
    // ✅ 应该保持任务简短
    PID_Update(...);  // 只需 100-200us
}
```

---

## 🏆 最佳实践

1. **CRITICAL 任务** → 使用标志位事件或周期性（< 1KHz）
2. **HIGH 任务** → 使用周期性（1-4KHz）
3. **NORMAL/LOW 任务** → 使用周期性或回调判断
4. **IDLE 任务** → 使用周期性（< 10Hz）

5. **中断驱动的数据处理** → 标志位事件
6. **固定频率控制回路** → 周期性
7. **按需处理（有数据才处理）** → 回调判断

希望这能帮助您理解三种注册方式的区别！🚁

