/*********************************************************************
 * NAN Marshal - Data type marshalling for NAN module
 *
 * Copyright (c) 2015 NAN-Marshal contributors:
 *   - Ievgen Khvedchenia <https://github.com/BloodAxe>
 *
 * MIT License <https://github.com/BloodAxe/nan-Marshal/blob/master/LICENSE.md>
 *
 * Version 0.0.1: current Node 4.0.0, Node 12: 0.12.7, Node 10: 0.10.40, iojs: 3.2.0
 *
 * See https://github.com/BloodAxe/nan-marshal for the latest update to this file
 **********************************************************************************/

#pragma once 

#include <nan.h>
#include <node_buffer.h>
#include <complex>
#include <functional>
#include <iostream>
#include <sstream>
#include <array>
#include <memory>
#include <map>
#include <stdexcept>

#if _MSC_VER
#define NANMARSHAL_NOTHROW _THROW0()
#else
#define NANMARSHAL_NOTHROW noexcept
#endif

namespace Nan
{
    typedef v8::Local<v8::Value> V8Result;

    template <typename T>
    T Marshal(V8Result val);

    template <typename T>
    V8Result Marshal(const T& val);

    class MarshalException : public std::runtime_error
    {
    public:

        const char * what() const NANMARSHAL_NOTHROW override
        {
              return m_message.c_str();
        }

        inline MarshalException(const char * message)
            : std::runtime_error("MarshalTypeMismatchException")
            , m_message(message)
        {
        }

    private:
        std::string m_message;
    };

    namespace marshal
    {
        struct access
        {
            template<typename Archive, typename T>
            static inline void serialize(Archive& ar, T& type)
            {
                type.serialize(ar);
            }
        };

        template <typename T>
        struct nvp_struct
        {
            const char * name;
            T& value;

            explicit inline nvp_struct(const char * name_, T & value_)
                : name(name_)
                , value(value_)
            {}

            inline nvp_struct(const nvp_struct & rhs)
                : name(rhs.name)
                , value(rhs.value)
            {}
        };

        template <typename T>
        inline nvp_struct<T> make_nvp(const char * name, T& val)
        {
            return nvp_struct<T>(name, val);
        }

        template <typename T>
        inline nvp_struct<T> make_nvp(const char * name, const T& val)
        {
            return nvp_struct<T>(name, const_cast<T&>(val));
        }

        template <typename T>
        inline nvp_struct<T> make_nvp(const char * name, T* val) = delete;

        template <typename T>
        inline nvp_struct<T> make_nvp(const char * name, const T* val) = delete;


        template<typename Archive, typename T>
        inline void serialize(Archive& ar, T& type)
        {
            access::serialize(ar, type);
        }

        template<typename T>
        struct Serializer
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, T& val)
            {
                marshal::serialize(ar, val);
            }
            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const T& val)
            {
                marshal::serialize(ar, const_cast<T&>(val));
            }
        };

#define BASIC_TYPE_SERIALIZER(type)\
    template<> \
        struct Serializer<type> \
        {\
        template<typename InputArchive>\
        static inline void load(InputArchive& ar, type& val)\
        {\
        ar.load(val); \
        }\
        template<typename OutputArchive>\
        static inline void save(OutputArchive& ar, const type& val)\
        {\
        ar.save(val); \
        }\
        }

#define ENUM_SERIALIZER(type)\
    template<>\
        struct Serializer<type>\
        {\
        template<typename InputArchive>\
        static inline void load(InputArchive& ar, type& val)\
        {\
        int int_val; \
        ar & int_val; \
        val = (type)int_val; \
        }\
        template<typename OutputArchive>\
        static inline void save(OutputArchive& ar, const type& val)\
        {\
        int int_val = (int)val; \
        ar & int_val; \
        }\
        }

        // declare serializers for simple types

        BASIC_TYPE_SERIALIZER(char);
        BASIC_TYPE_SERIALIZER(unsigned char);
        BASIC_TYPE_SERIALIZER(short);
        BASIC_TYPE_SERIALIZER(unsigned short);
        BASIC_TYPE_SERIALIZER(int);
        BASIC_TYPE_SERIALIZER(unsigned int);
        BASIC_TYPE_SERIALIZER(long);
        BASIC_TYPE_SERIALIZER(unsigned long);
        BASIC_TYPE_SERIALIZER(unsigned long long);
        BASIC_TYPE_SERIALIZER(float);
        BASIC_TYPE_SERIALIZER(double);
        BASIC_TYPE_SERIALIZER(bool);

        template<typename T>
        struct Serializer< std::shared_ptr<T> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::shared_ptr<T>& val)
            {
                val.reset(new T);
                T& _val = *val.get();
                ar & _val;
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::shared_ptr<T>& val)
            {
                const T& _val = *val.get();
                ar & _val;
            }
        };

        // serializer for std::vector
        template<typename T>
        struct Serializer < std::vector<T> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::vector<T>& val)
            {
                int N = ar.template As<v8::Array>()->Length();

                val.resize(N);
                for (int i = 0; i < N; i++)
                {
                    V8Result item = ar.template As<v8::Array>()->Get(i);
                    val[i] = Marshal<T>(item);
                }
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::vector<T>& val)
            {
                auto result = Nan::New<v8::Array>((int)val.size());

                for (uint32_t i = 0; i < val.size(); i++)
                {
                    const T& item = val[i];
                    result->Set(i, Marshal(item));
                }

                ar = result;
            }
        };

        // serializer for std::pair
        template<typename K, typename V>
        struct Serializer < std::pair<K, V> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::pair<K, V>& val)
            {
                ar & make_nvp("key", val.first);
                ar & make_nvp("value", val.second);
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::pair<K, V>& val)
            {
                ar & make_nvp("key", val.first);
                ar & make_nvp("value", val.second);
            }
        };

        template<>
        struct Serializer < std::string >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::string& val)
            {
                size_t len = Nan::DecodeBytes(ar, Nan::ASCII);
                val.resize(len);
                Nan::DecodeWrite(const_cast<char*>(val.data()), len, ar, Nan::ASCII);
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::string& val)
            {
                ar = Nan::New<v8::String>(val).ToLocalChecked();
            }
        };



        // serializer for std::map
        template<typename K, typename V>
        struct Serializer < std::map<K, V> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::map<K, V>& map_val)
            {
                int N = ar.template As<v8::Array>()->Length();
                for (int i = 0; i < N; i++)
                {
                    V8Result item = ar.template As<v8::Array>()->Get(i);
                    map_val.insert(Marshal< std::pair<K, V> >(item));
                }
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::map<K, V>& map_val)
            {
                v8::Local<v8::Array> result = Nan::New<v8::Array>();
                uint32_t idx = 0;
                for (typename std::map<K, V>::const_iterator i = map_val.begin(); i != map_val.end(); ++i, ++idx)
                {
                    result->Set(idx, Marshal(*i));
                }
                ar = result;
            }
        };

        // serializer for std::complex
        template<typename T>
        struct Serializer < std::complex<T> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::complex<T>& val)
            {
                T real, imag;
                ar & make_nvp("real", real);
                ar & make_nvp("imag", imag);

                val = std::complex<T>(real, imag);
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::complex<T>& val)
            {
                T real = val.real();
                T imag = val.imag();
                ar & make_nvp("real", real);
                ar & make_nvp("imag", imag);
            }
        };

        template<typename T>
        struct Serializer < nvp_struct<T> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, nvp_struct<T>& val)
            {
                ar.load(val);
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const nvp_struct<T>& val)
            {
                ar.save(val);
            }
        };

        template<typename T, int N>
        struct Serializer < T[N] >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, T(&val)[N])
            {
                for (int i = 0; i < N; i++)
                {
                    V8Result item = ar.template As<v8::Array>()->Get(i);
                    val[i] = Marshal<T>(item);
                }
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, T const (&val)[N])
            {
                v8::Local<v8::Array> result = Nan::New<v8::Array>(N);

                for (uint32_t i = 0; i < N; i++)
                {
                    const T& item = val[i];
                    result->Set(i, Marshal(item));
                }

                ar = result;
            }
        };

        template<typename T, std::size_t N>
        struct Serializer < std::array<T, N> >
        {
            template<typename InputArchive>
            static inline void load(InputArchive& ar, std::array<T, N>& val)
            {
                for (int i = 0; i < N; i++)
                {
                    V8Result item = ar.template As<v8::Array>()->Get(i);
                    val[i] = Marshal<T>(item);
                }
            }

            template<typename OutputArchive>
            static inline void save(OutputArchive& ar, const std::array<T, N>& val)
            {
                v8::Local<v8::Array> result = Nan::New<v8::Array>(static_cast<int>(N));

                for (uint32_t i = 0; i < N; i++)
                {
                    const T& item = val[i];
                    result->Set(i, Marshal(item));
                }

                ar = result;
            }
        };

        template <bool C_>
        struct bool_ {
            static const bool value = C_;
            typedef bool value_type;
        };

        class SaveArchive
        {
        public:

            typedef v8::Local<v8::Value> V8Result;

            inline SaveArchive()
            {
            }

            inline SaveArchive(V8Result& dst) : _dst(dst)
            {

            }

            inline ~SaveArchive()
            {

            }

            typedef bool_<false> is_loading;
            typedef bool_<true> is_saving;

            template<typename T>
            inline SaveArchive& operator& (const T& val)
            {
                Serializer<T>::save(*this, val);
                return *this;
            }

            template<typename T>
            inline void save(const T& val)
            {
                _dst = Nan::New(val);
            }

            template<typename T>
            inline void save(const nvp_struct<T>& val)
            {
                if (_dst.IsEmpty() || !_dst->IsObject())
                {
                    _dst = Nan::New<v8::Object>();
                }

                Nan::Set(_dst, Nan::New<v8::String>(val.name).ToLocalChecked(), Marshal(val.value));
            }

            template<typename T>
            void save(T* const& val) = delete;

            inline SaveArchive& operator=(V8Result newVal)
            {
                _dst = newVal;
                return *this;
            }

            inline operator V8Result()
            {
                return _dst;
            }

            V8Result _dst;

        private:
        };


        class LoadArchive
        {
        public:

            inline LoadArchive(V8Result src)
                : _src(src)
            {
            }

            inline ~LoadArchive()
            {
            }

            typedef bool_<true> is_loading;
            typedef bool_<false> is_saving;

            inline V8Result target() const
            {
                return _src;
            }

            template<typename T>
            inline LoadArchive& operator& (const T& val)
            {
                Serializer<T>::load(*this, const_cast<T&>(val));
                return *this;
            }

            template<typename T>
            void load(T& val);

            template<typename T>
            void load(T*& val) = delete;

            template<typename T>
            inline void load(nvp_struct<T>& val)
            {
                if (!_src->IsObject())
                {
                    throw MarshalException("Underlying instance is not an object");
                }

                auto prop = Nan::Get(_src, Nan::New<v8::String>(val.name).ToLocalChecked());
                if (prop.IsEmpty())
                {
                    throw MarshalException("Object does not contains property");
                }

                val.value = Marshal<T>(prop);
            }

            template <typename T>
            inline v8::Local<T> As()
            {
                return _src.template As<T>();
            }

            inline operator V8Result()
            {
                return _src;
            }

        private:
            V8Result _src;
        };

        template<>
        inline void LoadArchive::load(bool& val)
        {
            if (!_src->IsBoolean())
                throw MarshalException("Argument is not a boolean");

            //val = _src->BooleanValue(v8::Isolate::GetCurrent());
            val = Nan::To<bool>(_src).FromJust();
        }

        template<>
        inline void LoadArchive::load(int& val)
        {
            if (!_src->IsInt32())
                throw MarshalException("Argument is not a number");

            //val = _src->Int32Value(Nan::GetCurrentContext()).FromJust();
            val = Nan::To<int32_t>(_src).FromJust();
        }

        template<>
        inline void LoadArchive::load(float& val)
        {
            if (!_src->IsNumber())
                throw MarshalException("Argument is not a number");

            val = (float)_src->NumberValue(Nan::GetCurrentContext()).FromJust();
        }

        template<>
        inline void LoadArchive::load(double& val)
        {
            if (!_src->IsNumber())
                throw MarshalException("Argument is not a number");

            val = (double)_src->NumberValue(Nan::GetCurrentContext()).FromJust();
        }

        template<>
        inline void LoadArchive::load(uint32_t& val)
        {
            val = (uint32_t)_src->Uint32Value(Nan::GetCurrentContext()).FromJust();
        }

    }

    // Marshal functions implementation
    template <typename T>
    inline T Marshal(V8Result val)
    {
        marshal::LoadArchive ia(val);
        T loaded = T();
        ia & loaded;
        return std::move(loaded);
    }

    template <typename T>
    inline V8Result Marshal(const T& val)
    {
        Nan::EscapableHandleScope scope;

        marshal::SaveArchive oa;
        oa & val;

        return scope.Escape(oa._dst);
    }
}