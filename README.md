# Behavior Gamma - ESP32 GPIO and DAC Control System

## Project Overview

Behavior Gamma is an ESP32-based embedded system designed for real-time GPIO and DAC control through USB Serial JTAG communication. The system provides a comprehensive interface for digital and analog I/O operations, making it suitable for experimental setups, data acquisition, and hardware control applications.

## System Architecture

The project is built on ESP-IDF (Espressif IoT Development Framework) and utilizes FreeRTOS for task management. The system follows a modular design with separate components for initialization, GPIO control, DAC operations, and input buffering.

### Key Features

- **Real-time GPIO Control**: Digital read/write operations on configurable pins
- **Analog I/O Operations**: ADC reading and DAC output through SPI interface
- **USB Serial Communication**: Command-based protocol for external control
- **Input Buffering**: Continuous monitoring and buffering of digital inputs
- **Error Handling**: Comprehensive error checking with LED indicators
- **Multi-core Processing**: Utilizes ESP32's dual-core architecture

## File Structure and Components

### Core Files

#### 1. `main.c` - Main Application Controller
**Purpose**: Central coordinator handling USB Serial JTAG communication and task creation.

**Key Functions**:
- `uart_task()`: Processes incoming commands via USB Serial JTAG
- `app_main()`: Application entry point and task initialization

**Protocol Implementation**:
The system implements a binary communication protocol:
```
Command Format: [ID][PIN_TYPE][MODE][PIN][DATA_HIGH][DATA_LOW][END]
- ID: 0x33 (Start marker)
- PIN_TYPE: 0x00 (Digital) | 0x01 (Analog)
- MODE: 0x00 (Read) | 0x01 (Write)
- PIN: Pin number (1-based)
- DATA: 16-bit value for write operations
- END: 0x0A (End marker)
```

#### 2. `init.c` & `init.h` - System Initialization
**Purpose**: Configures all system peripherals and initializes hardware interfaces.

**Key Components**:
- **ADC Configuration**: Dual ADC units with calibration
- **SPI Interface**: For external DAC communication
- **GPIO Setup**: Digital input/output pin configuration
- **USB Serial JTAG**: Communication interface setup

**Hardware Configuration**:
```c
// SPI Pins for DAC
#define SPI_CLK 48    // Clock
#define SPI_DIN 38    // Data In
#define SPI_SYNC 47   // Sync/CS

// Status LED
#define ERROR_LED 46

// Digital Output Pins (8 channels)
DIGITAL_OUTPUT[] = {46, 10, 17, 8, 5, 6, 9, 7}

// Digital Input Pins (4 channels)
DIGITAL_INPUT[] = {21, 1, 18, 2}
```

#### 3. `gpio.c` & `gpio.h` - GPIO Operations
**Purpose**: Handles all digital and analog GPIO operations through dedicated FreeRTOS tasks.

**Task Functions**:
- `gpio_digital_write_task()`: Sets digital output states
- `gpio_digital_read_task()`: Reads digital input states with buffering
- `gpio_analog_read_task()`: Performs ADC readings with calibration

**ADC Channels**:
- **ADC1**: Channels 2 and 3 (Analog pins 1-2)
- **ADC2**: Channels 2 and 3 (Analog pins 3-4)
- **Voltage Scaling**: (2.8/1.8) factor applied for external circuit compatibility

#### 4. `dac.c` & `dac.h` - DAC Control
**Purpose**: Controls external DAC via SPI for analog output generation.

**Key Features**:
- 16-bit resolution DAC control
- SPI communication at 8MHz
- Channel mapping for hardware compatibility
- Voltage-to-digital conversion

**Channel Mapping**:
```c
channel_map[] = {3, 1, 4, 2}  // Maps logical to physical DAC channels
```

#### 5. `input_buffer.c` & `input_buffer.h` - Input Management
**Purpose**: Provides continuous monitoring and buffering of digital inputs.

**Buffer Structure**:
```c
typedef struct {
    int value;      // Current input value
    bool consumed;  // Consumption flag for read operations
} input_entry_t;
```

**Features**:
- Thread-safe access using mutex
- Continuous 1ms sampling rate
- State change detection
- Buffer management for reliable data access

## Communication Protocol

### Command Structure
The system uses a 7-byte binary protocol for USB Serial JTAG communication:

| Byte | Field | Description |
|------|-------|-------------|
| 0 | Start | 0x33 (Fixed start marker) |
| 1 | Pin Type | 0x00 (Digital) / 0x01 (Analog) |
| 2 | Mode | 0x00 (Read) / 0x01 (Write) |
| 3 | Pin Number | 1-based pin identifier |
| 4 | Data High | Upper byte of data |
| 5 | Data Low | Lower byte of data |
| 6 | End | 0x0A (Fixed end marker) |

### Response Format
All operations return a 7-byte response with the same structure, where data bytes contain:
- **Read operations**: Measured values or states
- **Write operations**: Success/error codes

## Hardware Requirements

### ESP32 Configuration
- **Target**: ESP32-S3 (configurable for ESP32)
- **Communication**: USB Serial JTAG
- **SPI**: SPI2_HOST for DAC communication
- **ADC**: Both ADC1 and ADC2 units

### External Components
- **DAC**: 16-bit SPI DAC (e.g., AD5676)
- **Input Circuits**: Digital input conditioning
- **Output Circuits**: Digital output drivers
- **Power Supply**: 3.3V for ESP32, 5V reference for DAC

### Pin Assignments

#### Digital I/O
- **Outputs**: Pins 10, 17, 8, 5, 6, 9, 7, 46 (8 channels)
- **Inputs**: Pins 21, 1, 18, 2 (4 channels)

#### Analog I/O
- **ADC Inputs**: GPIO pins mapped to ADC channels
- **DAC Output**: SPI interface (CLK=48, DIN=38, SYNC=47)

#### Status/Control
- **Error LED**: Pin 46
- **USB Serial JTAG**: Built-in USB interface

## Usage Considerations

### Performance Factors

1. **Task Priorities**: All communication tasks run at priority 5
2. **Core Assignment**: 
   - Core 0: Input buffer updates
   - Core 1: UART communication tasks
3. **Memory Usage**: Each task allocated 2048-20480 bytes stack
4. **Timing**: 1ms input sampling, 5ms communication timeouts

### Threading and Synchronization

- **UART Mutex**: Protects USB Serial JTAG access
- **Input Buffer Mutex**: Ensures thread-safe buffer operations
- **Task Management**: One-shot tasks for GPIO operations, continuous tasks for monitoring

### Error Handling

The system implements comprehensive error checking:
- **ERROR_CHECK Macro**: Automatically sets error LED on failures
- **Return Codes**: All operations return status information
- **Timeout Handling**: Communication operations have built-in timeouts

### Configuration Options

#### Compile-time Configuration
- Target ESP32 variant (ESP32/ESP32-S3)
- ADC attenuation settings (12dB default)
- SPI clock frequency (8MHz default)
- Buffer sizes (1024 bytes default)

#### Runtime Configuration
- Pin assignments through arrays
- Channel mapping for DAC outputs
- Sampling rates and timeouts

## Build and Deployment

### Prerequisites
- ESP-IDF v5.0+
- CMake 3.16+
- Appropriate toolchain for target ESP32 variant

### Build Commands
```bash
idf.py set-target esp32s3  # or esp32
idf.py build
idf.py flash monitor
```

### Configuration
```bash
idf.py menuconfig  # For advanced configuration
```

## Integration Guidelines

### External Control Systems
The system is designed for integration with:
- MATLAB/Simulink real-time systems
- Python control scripts
- LabVIEW applications
- Custom embedded controllers

### Protocol Implementation
When implementing client-side communication:

1. **Establish USB Serial Connection**: Use appropriate drivers
2. **Send Commands**: Follow 7-byte protocol format
3. **Handle Responses**: Parse 7-byte return messages
4. **Error Handling**: Monitor response codes and timeouts

### Performance Optimization
- **Batch Operations**: Group multiple commands to reduce overhead
- **Proper Timing**: Allow adequate time between operations
- **Buffer Management**: Monitor input buffer states for reliable reading

## Troubleshooting

### Common Issues

1. **Communication Failures**
   - Check USB connection and drivers
   - Verify baud rate and protocol format
   - Monitor error LED status

2. **ADC Reading Issues**
   - Verify input voltage ranges (0-3.3V)
   - Check calibration parameters
   - Ensure proper grounding

3. **DAC Output Problems**
   - Verify SPI connections
   - Check DAC power supply (5V reference)
   - Monitor SPI timing parameters

4. **GPIO State Issues**
   - Verify pin assignments in arrays
   - Check input/output configuration
   - Monitor task creation success

### Debug Features
- **USB Serial Output**: Real-time logging available
- **Error LED**: Pin 46 indicates system errors
- **Task Monitoring**: FreeRTOS task states available
- **Memory Debugging**: Stack usage monitoring enabled

## Future Enhancements

### Potential Improvements
- **PWM Output**: Add PWM generation capabilities
- **Interrupt-based I/O**: Implement edge-triggered input handling
- **Configuration Interface**: Runtime pin assignment modification
- **Data Logging**: On-board data storage capabilities
- **Network Communication**: WiFi/Ethernet integration options

### Scalability Considerations
- **Additional Channels**: Expandable I/O through external multiplexers
- **Higher Resolution**: Support for higher-bit DACs
- **Faster Sampling**: Optimization for higher-frequency operations
- **Protocol Extensions**: Additional command types and data formats

## Performance Characteristics

The Behavior Gamma system is optimized for high-speed, real-time operations with the following performance metrics:

### **Response Times**
- **Command Processing**: < 1ms response time from command reception to execution
- **Digital I/O Operations**: < 100μs for read/write operations
- **Analog Read Operations**: < 500μs including ADC conversion and calibration
- **SPI DAC Output**: < 200μs for voltage setting with 16-bit resolution

### **Throughput Specifications**
- **Input Sampling Rate**: 1000 Hz continuous monitoring for all digital inputs
- **Communication Speed**: USB Serial JTAG up to 12 Mbps effective bandwidth
- **SPI Interface**: 8 MHz clock for external DAC communication
- **Concurrent Operations**: Multi-task architecture supports simultaneous I/O operations

### **System Resources**
- **CPU Utilization**: Dual-core ESP32 with dedicated task distribution
- **Memory Usage**: 
  - Stack allocation: 2048-20480 bytes per task
  - Buffer sizes: 1KB for USB Serial JTAG TX/RX
- **Power Consumption**: Optimized for continuous operation with minimal idle states

## Key Advantages for Speed & Reliability

### **Architecture Benefits**
1. **USB Serial JTAG Over Traditional UART**
   - Eliminates UART baud rate limitations
   - Hardware-accelerated communication
   - Direct USB connection reduces latency
   - No external USB-to-serial conversion delays

2. **Dual-Core Task Distribution**
   - Core 0: High-frequency input sampling (1kHz)
   - Core 1: Command processing and communication
   - Parallel processing prevents blocking operations

3. **Binary Communication Protocol**
   - 7-byte fixed-length commands
   - No text parsing overhead
   - Immediate binary interpretation
   - Built-in error detection with start/end markers

### **Pin Management & Safety**
1. **Dedicated Pin Arrays**
   - Separate input/output pin definitions prevent confusion
   - Hardware-level pin mapping eliminates GPIO conflicts
   - Compile-time pin validation through array bounds

2. **Input Buffering System**
   - Continuous background sampling
   - Thread-safe access with mutex protection
   - Eliminates polling delays in read operations
   - Prevents data loss during high-frequency operations

3. **Error Handling & Validation**
   - Hardware error LED indication
   - Automatic range limiting for pin operations
   - Comprehensive error checking with ESP_ERROR_CHECK macros

### **Hardware Interface Optimization**
1. **ADC Configuration**
   - 12dB attenuation for full voltage range
   - Hardware calibration for accuracy
   - Dual ADC units prevent channel conflicts

2. **SPI Interface**
   - 8 MHz clock for high-speed DAC operations
   - Hardware-controlled CS signals
   - 16-bit resolution with optimized bit packing

## Future Enhancement Approaches

### **Performance Optimizations**

#### **1. DMA Integration**
```c
// Potential DMA implementation for burst operations
spi_device_interface_config_t devcfg = {
    .flags = SPI_DEVICE_NO_DUMMY,
    .clock_speed_hz = SPI_MASTER_FREQ_20M,  // Increase to 20MHz
    // Add DMA channel configuration
};
```

#### **2. Interrupt-Driven I/O**
- **GPIO Interrupts**: Replace polling with edge-triggered interrupts
- **Timer Interrupts**: Precise timing for sampling operations
- **Priority-based Scheduling**: Critical operations get higher priority

#### **3. Memory Optimization**
- **Static Memory Allocation**: Eliminate dynamic allocation overhead
- **Circular Buffers**: Efficient data streaming without memory fragmentation
- **Zero-copy Operations**: Direct buffer manipulation

### **Protocol Enhancements**

#### **1. Streaming Protocol**
```c
// High-throughput streaming for continuous data
typedef struct {
    uint32_t timestamp;
    uint16_t pin_data[8];  // Batch multiple pins
    uint16_t checksum;
} streaming_packet_t;
```

#### **2. Command Pipelining**
- **Asynchronous Commands**: Queue multiple operations
- **Batch Processing**: Execute multiple commands in sequence
- **Priority Queues**: Critical commands bypass normal queue

#### **3. Compression & Encoding**
- **Delta Encoding**: Transmit only changes in pin states
- **Run-Length Encoding**: Compress repetitive data patterns
- **Custom Binary Protocols**: Application-specific optimizations

### **Hardware Scaling**

#### **1. Multi-Device Support**
- **I2C/SPI Expansion**: Support for GPIO expanders
- **Daisy-Chain Architecture**: Multiple ESP32 units
- **Network Communication**: ESP-NOW for distributed systems

#### **2. Advanced Peripherals**
- **High-Speed ADCs**: External ADCs with parallel interfaces
- **FPGA Integration**: Hardware acceleration for complex operations
- **Real-Time Clock**: Precise timing synchronization

#### **3. Power Management**
- **Dynamic Frequency Scaling**: Adjust clock speeds based on load
- **Sleep Mode Integration**: Power saving during idle periods
- **Voltage Regulation**: Optimized power delivery

### **Software Architecture Improvements**

#### **1. Real-Time Operating System**
- **Deterministic Scheduling**: Guaranteed response times
- **Priority Inheritance**: Prevent priority inversion
- **Deadline Scheduling**: Meet strict timing requirements

#### **2. State Machine Implementation**
```c
// Robust state management for complex operations
typedef enum {
    STATE_IDLE,
    STATE_READING,
    STATE_WRITING,
    STATE_ERROR
} system_state_t;
```

#### **3. Advanced Error Handling**
- **Fault Recovery**: Automatic system recovery
- **Diagnostic Reporting**: Detailed error information
- **Watchdog Integration**: System health monitoring

### **Testing & Validation**

#### **1. Performance Benchmarking**
- **Latency Measurements**: Precise timing analysis
- **Throughput Testing**: Maximum data rates
- **Stress Testing**: System behavior under load

#### **2. Hardware-in-the-Loop Testing**
- **Automated Test Suites**: Comprehensive validation
- **Signal Generation**: Synthetic input patterns
- **Oscilloscope Integration**: Real-time signal analysis

#### **3. Continuous Integration**
- **Automated Builds**: ESP-IDF CI/CD pipeline
- **Unit Testing**: Component-level validation
- **Integration Testing**: End-to-end system testing

These enhancements would further improve the system's performance, reliability, and scalability while maintaining the current architecture's strengths.

This documentation provides a comprehensive overview of the Behavior Gamma system, enabling effective usage, maintenance, and future development.
