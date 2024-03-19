#include <px4_platform_common/module.h>
#include <px4_platform_common/px4_config.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <poll.h> // Include poll.h header for struct pollfd
#include <uORB/uORB.h>
#include <string.h> // for memset and memcpy

extern "C" __EXPORT int pwm_test_main(int argc, char *argv[]);

bool isValidNumber(int num) {
    return num >= -7500 && num <= 7500;
}

bool isValidSequence(const int sequence[], int length) {
    if (length != 4) return false;

    for (int i = 0; i < length; i++) {
        if (!isValidNumber(sequence[i])) return false;
    }
    return true;
}

int pwm_test_main(int argc, char *argv[]) {
    const char *device = "/dev/ttyS1"; // Change this to the appropriate serial port
    int serial_fd = ::open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (serial_fd < 0) {
        PX4_ERR("Failed to open serial port.");
        return -1;
    }

    //serial_init
    struct termios uart_config;
    tcgetattr(serial_fd, &uart_config);
    cfsetspeed(&uart_config, B9600);
    uart_config.c_cflag |= (CLOCAL | CREAD);
    uart_config.c_cflag &= ~CSIZE;
    uart_config.c_cflag |= CS8;
    uart_config.c_cflag &= ~PARENB;
    uart_config.c_cflag &= ~CSTOPB;
    tcsetattr(serial_fd, TCSANOW, &uart_config);

    // Setup poll to monitor user input
    struct pollfd fds;
    fds.fd = 0; // STDIN
    fds.events = POLLIN;

    // Example sequence
    int sequence[4] = {1000, 1000, 1000, 1000};

    // Validate sequence
    if (!isValidSequence(sequence, 4)) {
        PX4_ERR("Invalid sequence.");
        ::close(serial_fd);
        return -1;
    }

    char user_input = '\0';

    // Continuously send sequence until 'x' is received
    while(user_input != 'x') {
        size_t bytes_written = ::write(serial_fd, sequence, sizeof(sequence));

        if (bytes_written != sizeof(sequence)) {
            PX4_ERR("Failed to write the complete sequence.");
            break; // Exit loop if write fails
        }

        // Check if there is user input without blocking
        if(poll(&fds, 1, 0) > 0) {
            read(0, &user_input, 1); // Read one char from stdin
        }

        usleep(100000); // Sleep for 0.1 seconds before sending again
    }

    ::close(serial_fd);

    return 0;
}
