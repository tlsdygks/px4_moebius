#include <px4_platform_common/module.h>
#include <px4_platform_common/px4_config.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

extern "C" __EXPORT int serial_test_main(int argc, char *argv[]);

int serial_test_main(int argc, char *argv[]) {
    const char *device = "/dev/ttyS1"; // Change this to the appropriate serial port
    int serial_fd = ::open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (serial_fd < 0) {
        PX4_ERR("Failed to open serial port.");
        return -1;
    }

    struct termios uart_config;
    tcgetattr(serial_fd, &uart_config);
    cfsetspeed(&uart_config, B9600);
    uart_config.c_cflag |= (CLOCAL | CREAD);
    uart_config.c_cflag &= ~CSIZE;
    uart_config.c_cflag |= CS8;
    uart_config.c_cflag &= ~PARENB;
    uart_config.c_cflag &= ~CSTOPB;
    tcsetattr(serial_fd, TCSANOW, &uart_config);

    while (true) {
        // Prompt user for input
        printf("Enter a character to send: ");
        fflush(stdout); // Make sure the prompt is printed

        char input;
        scanf("%c", &input);

        // Send the input character
        ssize_t bytes_written = ::write(serial_fd, &input, 1);
        
        if (bytes_written < 0) {
            PX4_ERR("Error writing to serial port.");
            break;
        } else if (bytes_written == 0) {
            PX4_WARN("No data written to serial port.");
        } else {
            PX4_INFO("Sent '%c' to serial port.", input);
        }

        // Clear input buffer
        while ((getchar()) != '\n'); // Clear any remaining characters in the input buffer
    }

    ::close(serial_fd);

    return 0;
}

