#ifndef RX_CORE_PROFILER_H
#define RX_CORE_PROFILER_H
#include "rx/core/global.h"
#include "rx/core/optional.h"

namespace Rx {

struct Profiler {
  struct CPUSample {
    CPUSample(const char* _tag);
    ~CPUSample();
  };

  struct GPUSample {
    GPUSample(const char* _tag);
    ~GPUSample();
  };

  struct Device {
    typedef void (*SetThreadNameFn)(void* _context, const char* _name);
    typedef void (*BeginSampleFn)(void* _context, const char* _tag);
    typedef void (*EndSampleFn)(void* _context);

    constexpr Device(void* _context, SetThreadNameFn _set_thread_name_fn,
      BeginSampleFn _begin_sample_fn, EndSampleFn _end_sample_fn);

  private:
    friend struct Profiler;

    void* m_context;
    SetThreadNameFn m_set_thread_name_fn;
    BeginSampleFn m_begin_sample_fn;
    EndSampleFn m_end_sample_fn;
  };

  using CPU = Device;
  using GPU = Device;

  void set_thread_name(const char* _name);

  void bind_gpu(const GPU& _gpu);
  void bind_cpu(const CPU& _cpu);

  void unbind_gpu();
  void unbind_cpu();

  static constexpr Profiler& instance();

private:
  friend struct CPUSample;
  friend struct GPUSample;

  void begin_cpu_sample(const char* _tag);
  void end_cpu_sample();

  void begin_gpu_sample(const char* _tag);
  void end_gpu_sample();

  Optional<GPU> m_gpu;
  Optional<CPU> m_cpu;

  static Global<Profiler> s_instance;
};

inline Profiler::CPUSample::CPUSample(const char* _tag) {
  instance().begin_cpu_sample(_tag);
}

inline Profiler::CPUSample::~CPUSample() {
  instance().end_cpu_sample();
}

inline Profiler::GPUSample::GPUSample(const char* _tag) {
  instance().begin_gpu_sample(_tag);
}

inline Profiler::GPUSample::~GPUSample() {
  instance().end_gpu_sample();
}

inline constexpr Profiler::Device::Device(void* _context,
  SetThreadNameFn _set_thread_name_fn, BeginSampleFn _begin_sample_fn,
  EndSampleFn _end_sample_fn)
  : m_context{_context}
  , m_set_thread_name_fn{_set_thread_name_fn}
  , m_begin_sample_fn{_begin_sample_fn}
  , m_end_sample_fn{_end_sample_fn}
{
  RX_ASSERT(m_set_thread_name_fn, "thread name function missing");
  RX_ASSERT(m_begin_sample_fn, "begin sample function missing");
  RX_ASSERT(m_end_sample_fn, "end sample function missing");
}

inline void Profiler::set_thread_name(const char* _name) {
  if (m_gpu) {
    m_gpu->m_set_thread_name_fn(m_gpu->m_context, _name);
  }

  if (m_cpu) {
    m_cpu->m_set_thread_name_fn(m_cpu->m_context, _name);
  }
}

inline void Profiler::bind_gpu(const GPU& _gpu) {
  m_gpu = _gpu;
}

inline void Profiler::bind_cpu(const CPU& _cpu) {
  m_cpu = _cpu;
}

inline void Profiler::unbind_gpu() {
  m_gpu = nullopt;
}

inline void Profiler::unbind_cpu() {
  m_cpu = nullopt;
}

inline constexpr Profiler& Profiler::instance() {
  return *s_instance;
}

inline void Profiler::begin_cpu_sample(const char* _tag) {
  if (m_cpu) {
    m_cpu->m_begin_sample_fn(m_cpu->m_context, _tag);
  }
}

inline void Profiler::end_cpu_sample() {
  if (m_cpu) {
    m_cpu->m_end_sample_fn(m_cpu->m_context);
  }
}

inline void Profiler::begin_gpu_sample(const char* _tag) {
  if (m_gpu) {
    m_gpu->m_begin_sample_fn(m_gpu->m_context, _tag);
  }
}

inline void Profiler::end_gpu_sample() {
  if (m_gpu) {
    m_gpu->m_end_sample_fn(m_gpu->m_context);
  }
}

} // namespace rx

#endif // RX_PROFILER_H
