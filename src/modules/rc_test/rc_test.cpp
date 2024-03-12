#include <px4_platform_common/module.h>
#include <px4_platform_common/px4_config.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <poll.h> // Include poll.h header for struct pollfd
#include <uORB/uORB.h>
#include <uORB/topics/rc_channels.h>

extern "C" __EXPORT int rc_test_main(int argc, char *argv[]);


int rc_test_main(int argc, char *argv[]) {
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

    // Subscribe to RC channels topic
    int rc_sub_fd = orb_subscribe(ORB_ID(rc_channels));

    // Set up polling
    struct pollfd fds[1];
    fds[0].fd = rc_sub_fd;
    fds[0].events = POLLIN;

    bool should_exit = false;

    while (!should_exit) {
        // Wait for data to become available
        int ret = poll(fds, 1, 1000);

        if (ret < 0) {
            PX4_ERR("poll error");
            break;
        }

        if (fds[0].revents & POLLIN) {
            // Get RC channels data
            struct rc_channels_s rc_channels_data;
            orb_copy(ORB_ID(rc_channels), rc_sub_fd, &rc_channels_data);

            // Determine which character to send based on the pitch and yaw values
            char input;
            if (rc_channels_data.channels[2] > 0.5f && rc_channels_data.channels[3] > 0.5f) {
                input = 'H'; // Pitch and Yaw > 0.5: Send 'H'
            }
            else if (rc_channels_data.channels[2] < -0.5f && rc_channels_data.channels[3] < -0.5f) {
                input = 'D'; // Pitch and Yaw < -0.5: Send 'D'
            }
            else if (rc_channels_data.channels[2] > 0.5f && rc_channels_data.channels[3] < -0.5f) {
                input = 'B'; // Pitch > 0.5 and Yaw < -0.5: Send 'B'
            }
            else if (rc_channels_data.channels[2] < -0.5f && rc_channels_data.channels[3] > 0.5f) {
                input = 'F'; // Pitch < -0.5 and Yaw > 0.5: Send 'F'
            }
            else if (rc_channels_data.channels[2] > 0.5f) {
                input = 'A'; // Pitch > 0.5: Send 'A'
            }
            else if (rc_channels_data.channels[2] < -0.5f) {
                input = 'E'; // Pitch < -0.5: Send 'E'
            }
            else if (rc_channels_data.channels[3] > 0.5f) {
                input = 'G'; // Yaw > 0.5: Send 'C'
            }
            else if (rc_channels_data.channels[3] < -0.5f) {
                input = 'C'; // Yaw < -0.5: Send 'G'
            }
            else {
                input = 'Z'; // Otherwise: Send 'Z'
            }

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
        }
        // Check for user input to exit
        char exit_char;
        if (::read(STDIN_FILENO, &exit_char, 1) > 0) {
            if (exit_char == 'x') {
                should_exit = true;
            }
        }

    }



    ::close(serial_fd);

    return 0;
}

