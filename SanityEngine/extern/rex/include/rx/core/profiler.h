#ifndef RX_CORE_PROFILER_H
#define RX_CORE_PROFILER_H
#include "rx/core/pp.h"
#include "rx/core/global.h"
#include "rx/core/optional.h"
#include "rx/core/source_location.h"

namespace Rx {

struct RX_API Profiler {
  struct Sample {
    constexpr Sample(const SourceLocation& _source_location, const char* _tag);
    ~Sample();

    // Enframe |T| in the sample.
    template<typename T, typename... Ts>
    T* enframe(Ts&&... _arguments) const;

    const SourceLocation& source_location() const &;
    const char* tag() const;

    // Extract enframed |T|.
    template<typename T>
    const T* enframing() const;

  private:
    const SourceLocation& m_source_location;
    const char* m_tag;

    // Side-band enframing of data for profilers, cache-line in sized.
    mutable void (*m_enframing_destruct)(void* _enframing);
    union {
      alignas(16) mutable Byte m_enframing[64];
      Utility::Nat m_nat;
    };
  };

  struct CPUSample : Sample {
    CPUSample(const SourceLocation& _source_location, const char* _tag);
    ~CPUSample();
  };

  struct GPUSample : Sample {
    GPUSample(const SourceLocation& _source_location, const char* _tag);
    ~GPUSample();
  };

  struct Device {
    typedef void (*SetThreadNameFn)(void* _context, const char* _name);
    typedef void (*BeginSampleFn)(void* _context, const Sample* _sample);
    typedef void (*EndSampleFn)(void* _context, const Sample* _sample);

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

  void begin_cpu_sample(const CPUSample* _sample);
  void end_cpu_sample(const CPUSample* _sample);

  void begin_gpu_sample(const GPUSample* _sample);
  void end_gpu_sample(const GPUSample* _sample);

  Optional<GPU> m_gpu;
  Optional<CPU> m_cpu;

  static Global<Profiler> s_instance;
};

// Sample
inline constexpr Profiler::Sample::Sample(const SourceLocation& _source_location, const char* _tag)
  : m_source_location{_source_location}
  , m_tag{_tag}
  , m_enframing_destruct{nullptr}
  , m_nat{}
{
}

inline Profiler::Sample::~Sample() {
  if (m_enframing_destruct) {
    m_enframing_destruct(reinterpret_cast<void*>(m_enframing));
  }
}

template<typename T, typename... Ts>
inline T* Profiler::Sample::enframe(Ts&&... _arguments) const {
  RX_ASSERT(!m_enframing_destruct, "already enframed");
  static_assert(sizeof(T) <= sizeof m_enframing, "too much data to enframe");
  m_enframing_destruct = &Utility::destruct<T>;
  return Utility::construct<T>(m_enframing, Utility::forward<Ts>(_arguments)...);
}

inline const SourceLocation& Profiler::Sample::source_location() const & {
  return m_source_location;
}

inline const char* Profiler::Sample::tag() const {
  return m_tag;
}

template<typename T>
inline const T* Profiler::Sample::enframing() const {
  return reinterpret_cast<const T*>(m_enframing);
}

// CPUSample
inline Profiler::CPUSample::CPUSample(const SourceLocation& _source_location, const char* _tag)
  : Sample{_source_location, _tag}
{
  instance().begin_cpu_sample(this);
}

inline Profiler::CPUSample::~CPUSample() {
  instance().end_cpu_sample(this);
}

// GPUSample
inline Profiler::GPUSample::GPUSample(const SourceLocation& _source_location, const char* _tag)
  : Sample{_source_location, _tag}
{
  instance().begin_gpu_sample(this);
}

inline Profiler::GPUSample::~GPUSample() {
  instance().end_gpu_sample(this);
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

inline void Profiler::begin_cpu_sample(const CPUSample* _sample) {
  if (m_cpu) {
    m_cpu->m_begin_sample_fn(m_cpu->m_context, _sample);
  }
}

inline void Profiler::end_cpu_sample(const CPUSample* _sample) {
  if (m_cpu) {
    m_cpu->m_end_sample_fn(m_cpu->m_context, _sample);
  }
}

inline void Profiler::begin_gpu_sample(const GPUSample* _sample) {
  if (m_gpu) {
    m_gpu->m_begin_sample_fn(m_gpu->m_context, _sample);
  }
}

inline void Profiler::end_gpu_sample(const GPUSample* _sample) {
  if (m_gpu) {
    m_gpu->m_end_sample_fn(m_gpu->m_context, _sample);
  }
}

} // namespace rx

#define RX_PROFILE_CPU(tag) \
  const ::Rx::Profiler::CPUSample RX_PP_UNIQUE(rx_profile){RX_SOURCE_LOCATION, (tag)}

#define RX_PROFILE_GPU(tag) \
  const ::Rx::Profiler::GPUSample RX_PP_UNIQUE(rx_profile){RX_SOURCE_LOCATION, (tag)}

#endif // RX_PROFILER_H
