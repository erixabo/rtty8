#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <librpitx/librpitx.h>

#define byte uint8_t

int MARK = 2125;
int SPACE = 1955;
int STOP_BITS = 2;
int BIT_TIME = 220022;      // default 45.45 baud

ngfmdmasync *fmmod;
int FifoSize = 10000;
bool running = true;

void playtone(double Frequency, uint32_t Timing) // Timing in 0.1us
{
    uint32_t SumTiming = 0;
    SumTiming += Timing % 100;
    if (SumTiming >= 100)
    {
        Timing += 100;
        SumTiming -= 100;
    }
    int NbSamples = (Timing / 100);

    while (NbSamples > 0)
    {
        usleep(10);
        int Available = fmmod->GetBufferAvailable();
        if (Available > FifoSize / 2)
        {
            int Index = fmmod->GetUserMemIndex();
            if (Available > NbSamples)
                Available = NbSamples;
            for (int j = 0; j < Available; j++)
            {
                fmmod->SetFrequencySample(Index + j, Frequency);
                NbSamples--;
            }
        }
    }
}

void rtty_txbit(bool bit)
{
    if (running)
    {
        if (bit)
            playtone(MARK, BIT_TIME);
        else
            playtone(SPACE, BIT_TIME);
    }
}

void rtty_txbyte(byte c)
{
    rtty_txbit(0); // start bit
    // 8 data bits, LSB first (standard for ASCII RTTY)
    for (int i = 0; i < 8; i++)
        rtty_txbit((c >> i) & 1);
    // stop bits
    for (int i = 0; i < STOP_BITS; i++)
        rtty_txbit(1);
}

void send_data(const unsigned char *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        rtty_txbyte(data[i]);
}

static void terminate(int num)
{
    running = false;
    fprintf(stderr, "Caught signal - Terminating %x\n", num);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("usage: pirtty <frequency(Hz)> <space_freq(Hz)> [baud] [stopbits] [shift]\n");
        printf("  space_freq : space frequency in Hz (mark = space + shift)\n");
        printf("  baud       : baud rate (float, default 50)\n");
        printf("  stopbits   : 1 or 2 (default 2)\n");
        printf("  shift      : mark-space shift in Hz (default 170)\n");
        printf("Then write data to /tmp/rttytx (FIFO) – binary data allowed\n");
        exit(0);
    }

    double frequency = atof(argv[1]);
    MARK = atoi(argv[2]);

    double baud_rate = 50;
    if (argc >= 4)
        baud_rate = atof(argv[3]);

    STOP_BITS = 2;
    if (argc >= 5)
        STOP_BITS = atoi(argv[4]);
    if (STOP_BITS < 1 || STOP_BITS > 2)
        STOP_BITS = 2;

    int shift = 170;
    if (argc >= 6)
        shift = atoi(argv[5]);

    SPACE = MARK - shift;

    // BIT_TIME in 0.1us units: 10^7 / baud_rate
    BIT_TIME = (int)round(10000000.0 / baud_rate);
    if (BIT_TIME < 1)
        BIT_TIME = 220022; // fallback

    // Signal handlers
    for (int i = 0; i < 64; i++)
    {
        struct sigaction sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate;
        sigaction(i, &sa, NULL);
    }

    // Create FIFO
    umask(0);
    const char *fifo_path = "/tmp/rttytx";
    unlink(fifo_path);
    if (mkfifo(fifo_path, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }
    printf("FIFO created: %s\n", fifo_path);
    printf("Waiting for messages... (baud=%.2f, stop=%d, shift=%d Hz)\n", baud_rate, STOP_BITS, shift);

    fmmod = new ngfmdmasync(frequency, 100000, 14, FifoSize);

    while (running)
    {
        int fd = open(fifo_path, O_RDONLY);
        if (fd == -1)
        {
            perror("open fifo");
            break;
        }

        unsigned char buffer[65536];
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        close(fd);

        if (bytes_read > 0)
        {
            printf("Sending %zd bytes\n", bytes_read);
            send_data(buffer, bytes_read);

            usleep(200000); // allow last samples to leave
            delete fmmod;
            fmmod = new ngfmdmasync(frequency, 100000, 14, FifoSize);
            printf("Message sent, waiting for next...\n");
        }
        else if (bytes_read == 0)
        {
            continue;
        }
        else
        {
            perror("read");
            break;
        }
    }

    delete fmmod;
    unlink(fifo_path);
    return 0;
}
