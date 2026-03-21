#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <librpitx/librpitx.h>

#define byte uint8_t

// Globális paraméterek (az argumentumokból beállítva)
int MARK = 2125;
int SPACE = 1955;
int STOP_BITS = 2;
int BIT_TIME = 220022;      // default 45.45 baud

const int FifoSize = 10000;

// Jelző a szálbiztos jelkezeléshez
static volatile sig_atomic_t running = 1;
static int self_pipe[2];    // pipe a jelkezelő és a select közötti kommunikációhoz

// RTTY adó függvények
void playtone(double Frequency, uint32_t Timing, ngfmdmasync *fmmod) // Timing in 0.1us
{
    if (!fmmod) return;

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

void rtty_txbit(bool bit, ngfmdmasync *fmmod)
{
    if (running)
    {
        if (bit)
            playtone(MARK, BIT_TIME, fmmod);
        else
            playtone(SPACE, BIT_TIME, fmmod);
    }
}

void rtty_txbyte(byte c, ngfmdmasync *fmmod)
{
    rtty_txbit(0, fmmod); // start bit
    for (int i = 0; i < 8; i++)
        rtty_txbit((c >> i) & 1, fmmod);
    for (int i = 0; i < STOP_BITS; i++)
        rtty_txbit(1, fmmod);
}

void send_data(const unsigned char *data, size_t len, ngfmdmasync *fmmod)
{
    for (size_t i = 0; i < len; i++)
        rtty_txbyte(data[i], fmmod);
}

// Jelkezelő: beállítja a running flaget és ír a pipe‑ba a select feloldásához
static void signal_handler(int)
{
    running = 0;
    write(self_pipe[1], "x", 1);   // egy bájt írása a pipe-ba
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
        printf("Reads messages from /tmp/rttytx, waits for STX (0x02) and EOT (0x04)\n");
        printf("Carrier is on only during transmission.\n");
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

    // Signal kezelés – csak a SIGINT és SIGTERM
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // FIFO létrehozása
    umask(0);
    const char *fifo_path = "/tmp/rttytx";
    unlink(fifo_path);
    if (mkfifo(fifo_path, 0666) == -1)
    {
        perror("mkfifo");
        exit(1);
    }

    // Self-pipe létrehozása
    if (pipe(self_pipe) == -1)
    {
        perror("pipe");
        exit(1);
    }

    printf("FIFO created: %s\n", fifo_path);
    printf("Waiting for messages (STX...EOT) ... (baud=%.2f, stop=%d, shift=%d Hz)\n",
           baud_rate, STOP_BITS, shift);
    printf("Carrier is off until a complete message is received.\n");

    // Állapotgép a FIFO olvasásához
    enum State { WAIT_STX, COLLECT };
    State state = WAIT_STX;
    unsigned char buffer[4096];
    size_t buf_len = 0;

    int fifo_fd = -1;
    fd_set readfds;
    int max_fd;

    while (running)
    {
        // Ha nincs nyitott FIFO, próbáljuk megnyitni
        if (fifo_fd < 0)
        {
            fifo_fd = open(fifo_path, O_RDONLY);
            if (fifo_fd < 0)
            {
                perror("open fifo");
                usleep(1000000);
                continue;
            }
            state = WAIT_STX;
            buf_len = 0;
        }

        // select előkészítése
        FD_ZERO(&readfds);
        FD_SET(fifo_fd, &readfds);
        FD_SET(self_pipe[0], &readfds);
        max_fd = (fifo_fd > self_pipe[0]) ? fifo_fd : self_pipe[0];

        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0)
        {
            if (errno == EINTR) continue;  // megszakítás, újrapróbál
            perror("select");
            break;
        }

        // Ha a self-pipe jelez, kilépünk
        if (FD_ISSET(self_pipe[0], &readfds))
        {
            char dummy;
            read(self_pipe[0], &dummy, 1);
            break;  // kilépés a ciklusból
        }

        // Ha a FIFO-ban van adat
        if (FD_ISSET(fifo_fd, &readfds))
        {
            unsigned char c;
            ssize_t n = read(fifo_fd, &c, 1);
            if (n <= 0)
            {
                // FIFO író oldala bezárt (pl. a shell script újraindult)
                close(fifo_fd);
                fifo_fd = -1;
                state = WAIT_STX;
                buf_len = 0;
                continue;
            }

            // Állapotgép feldolgozása
            switch (state)
            {
                case WAIT_STX:
                    if (c == 0x02)  // STX
                    {
                        state = COLLECT;
                        buf_len = 0;
                    }
                    break;

                case COLLECT:
                    if (c == 0x04)  // EOT
                    {
                        if (buf_len > 0)
                        {
                            // Vivő bekapcsolása: modulátor létrehozása
                            ngfmdmasync *fmmod = new ngfmdmasync(frequency, 100000, 14, FifoSize);
                            printf("Carrier ON, sending %zu bytes\n", buf_len);
                            send_data(buffer, buf_len, fmmod);
                            usleep(200000);   // minták kiürülése
                            delete fmmod;
                            printf("Carrier OFF, ready for next message\n");
                        }
                        state = WAIT_STX;
                    }
                    else
                    {
                        // Adatgyűjtés
                        if (buf_len < sizeof(buffer) - 1)
                            buffer[buf_len++] = c;
                        else
                        {
                            fprintf(stderr, "Message too long, discarding\n");
                            state = WAIT_STX;
                        }
                    }
                    break;
            }
        }
    }

    // Takarítás
    if (fifo_fd >= 0) close(fifo_fd);
    close(self_pipe[0]);
    close(self_pipe[1]);
    unlink(fifo_path);
    return 0;
}
