#include "param_freebot_g474_col.h"
#include "../motorlib/usb_communication.h"
#include "st_device.h"
#include "../motorlib/system.h"
#include "../motorlib/qep_encoder.h"
#include "../motorlib/peripheral/stm32g4/hrpwm.h"
#include "../motorlib/peripheral/spi_encoder.h"
#include "../motorlib/peripheral/stm32g4/ams_encoder.h"
#include "../motorlib/hall.h"
#include "../motorlib/fast_loop.h"
#include "../motorlib/main_loop.h"
#include "../motorlib/led.h"
#include "../motorlib/peripheral/usb.h"
#include "../motorlib/actuator.h"
#include "../motorlib/ma732_encoder.h"
#include "../motorlib/peripheral/stm32g4/spi_torque.h"
#include "../motorlib/phony_encoder.h"
#include "Inc/main.h"

typedef FastLoop<HRPWM, MA732Encoder> FastLoopConfig;
typedef MainLoop<FastLoopConfig> MainLoopConfig;
typedef Actuator<FastLoopConfig, MainLoopConfig> ActuatorConfig;
typedef System<ActuatorConfig, USB1> SystemConfig;

template<>
USB1 SystemConfig ::usb_ = {};
template<>
std::queue<std::string> SystemConfig ::log_queue_ = {};

static struct {
    uint32_t pwm_frequency = (double) CPU_FREQUENCY_HZ * 32.0 / (hrperiod);
    uint32_t main_loop_frequency = (double) CPU_FREQUENCY_HZ/(main_loop_period);
    GPIO motor_encoder_cs = {*GPIOB, 4, GPIO::OUTPUT};
    MA732Encoder motor_encoder = {*SPI3, motor_encoder_cs};
    GPIO torque_cs = {*GPIOA, 15, GPIO::OUTPUT};
    SPITorque torque_sensor = {*SPI1, torque_cs, *DMA1_Channel1, *DMA1_Channel2};
    GPIO output_encoder_cs = {*GPIOD, 2, GPIO::OUTPUT};
    //MA732Encoder output_encoder = {*SPI3, output_encoder_cs}; // need to make sure this doesn't collide with motor encoder
    PhonyEncoder output_encoder = {65536};
    GPIO enable = {*GPIOC, 11, GPIO::OUTPUT};
    HRPWM motor_pwm = {pwm_frequency, *HRTIM1, 4, 5, 3};
    FastLoopConfig fast_loop = {(int32_t) pwm_frequency, motor_pwm, motor_encoder, param->fast_loop_param};
    LED led = {const_cast<uint16_t*>(reinterpret_cast<volatile uint16_t *>(&TIM4->CCR1)), 
               const_cast<uint16_t*>(reinterpret_cast<volatile uint16_t *>(&TIM4->CCR2)),
               const_cast<uint16_t*>(reinterpret_cast<volatile uint16_t *>(&TIM4->CCR3))};
    PIDDeadbandController controller = {(float) (1.0/main_loop_frequency)};
    PIDController torque_controller = {(float) (1.0/main_loop_frequency)};
    PIDDeadbandController impedance_controller = {(float) (1.0/main_loop_frequency)};
    USBCommunication<USB1> communication = {SystemConfig::usb_};
    MainLoopConfig main_loop = {fast_loop, controller, torque_controller, impedance_controller, communication, led, output_encoder, torque_sensor, param->main_loop_param};
} config_items;

template<>
ActuatorConfig SystemConfig::actuator_ = {config_items.fast_loop, config_items.main_loop, param->startup_param};

void system_init() {
    if (config_items.motor_encoder.init()) {
        SystemConfig::log("Motor encoder init success");
    } else {
        SystemConfig::log("Motor encoder init failure");
    }
    config_items.torque_sensor.init();
}

#include "../motorlib/system.cpp"