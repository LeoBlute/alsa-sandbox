#include <alsa/asoundlib.h>
#include <math.h>
#include <sys/mman.h>

#define PI32 3.14159265358979323846f

void timestamp(const char* msg, clock_t* c) {
  const clock_t end = clock();
  const double time_spent = (double)(end - *c) / CLOCKS_PER_SEC;
  printf("%s-time: %f\n", msg, time_spent);
  *c = clock();
}

int main() {
   snd_output_t* output;
   snd_pcm_t* pcm;
   snd_output_stdio_attach(&output, stderr, 0);
   snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);

   snd_pcm_hw_params_t *hw_params;
   hw_params = mmap(0, snd_pcm_hw_params_sizeof(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   snd_pcm_hw_params_any(pcm, hw_params);

   // snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
   snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED);
   snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
   snd_pcm_hw_params_set_channels(pcm, hw_params, 2);
   snd_pcm_hw_params_set_rate(pcm, hw_params, 48000, 0);
   snd_pcm_hw_params_set_buffer_size(pcm, hw_params, 48000*2);
   // snd_pcm_hw_params_set_periods(pcm, hw_params, 10, 0);
   snd_pcm_hw_params_set_period_time(pcm, hw_params, 100000, 0); // 0.1 seconds

   snd_pcm_hw_params(pcm, hw_params);

   snd_pcm_dump(pcm, output);

   int samples_per_second = 48000;
   int bytes_per_sample = 2 * sizeof(int16_t);
   int tone_hz = 256;
   int square_wave_period = samples_per_second/tone_hz;
   int half_square_wave_period = square_wave_period/2;
   int wave_period = samples_per_second/tone_hz;
   uint32_t running_sample_index = 0;
   int tone_volume = 5000;
   int samples_per_write = samples_per_second/8;
   printf("%d\n", samples_per_write);

   int16_t* samples = (int16_t*)mmap(0,
                                     samples_per_write*bytes_per_sample,
                                     PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS,
                                     -1,
                                     0);
   int16_t* sample_out = samples;

#if 0
   for (int i = 0; i < samples_per_write; i++) {
      int16_t sample_value = 10000 * sinf(2 * PI32 * tone_hz * ((float)i / samples_per_write));
      *sample_out++ = sample_value;
      *sample_out++ = sample_value;
   }
#endif
   while(1) {
      snd_pcm_sframes_t delay, avail;
      snd_pcm_avail_delay(pcm, &avail, &delay);
      usleep(120000);

      if(avail >= (samples_per_write)) {
         printf("%ld, %ld\n", avail, delay);
         clock_t timer = clock();
         sample_out = samples;
#if 0
         for(int i=0; i<samples_per_write; ++i) {
            int16_t sample_value = ((running_sample_index / (half_square_wave_period)) % 2) ? tone_volume : -tone_volume;
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            ++running_sample_index;
         }
#else
         for(int i=0; i<samples_per_write; ++i) {
            float t = 2.0f*PI32*(float)running_sample_index / (float)wave_period;
            float sine_value = sinf(t);
            int16_t sample_value = (int16_t)(sine_value * tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            ++running_sample_index;
         }
#endif
         while(snd_pcm_writei(pcm, samples, samples_per_write) == -EPIPE) {
            snd_pcm_prepare(pcm);
            printf("here\n");
         }
         // timestamp("time", &timer);
      }
   }

   snd_pcm_drain(pcm);
   snd_pcm_close(pcm);
}
