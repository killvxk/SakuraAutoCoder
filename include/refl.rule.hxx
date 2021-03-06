﻿/*
 * @Author: your name
 * @Date: 2020-04-04 12:32:09
 * @LastEditTime: 2020-04-16 22:31:47
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \undefinedd:\Coding\SakuraAutoCoder\CodeGen\refl.rule.hxx
 */
#pragma once
#include <atomic>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <map>
#include <vector>
#include <cassert>

#include "boost/hana.hpp"
namespace hana = boost::hana;
using namespace hana::literals;
using namespace std;
namespace std::pmr
{
	struct string;
}

namespace Sakura::refl
{
	class Object;
	class Reference;
	class IType;
}

#define SFIELD_INFO(NAME, T, MOTHER_BOARD, ...) \
	struct NAME##_info{\
		constexpr NAME##_info() = default;\
		Field fd = Field{\
			#NAME, #T, offsetof(MOTHER_BOARD, NAME), __VA_ARGS__ };\
		decltype(&MOTHER_BOARD::NAME) ptr = &MOTHER_BOARD::NAME;\
	};

#define SMETHOD_INFO(NAME, T, MOTHER_BOARD, ...) \
	struct NAME##_info{\
		constexpr NAME##_info() = default;\
		Field fd = Field{#NAME, #T, 0, __VA_ARGS__ };\
		decltype(&MOTHER_BOARD::NAME) ptr = &MOTHER_BOARD::NAME;\
	};

#define SSTATICFIELD_INFO(NAME, T, MOTHER_BOARD, ...)  \
	struct NAME##_info{\
		constexpr NAME##_info() = default;\
		Field fd = Field{#NAME, #T, 0, __VA_ARGS__ };\
		decltype(&MOTHER_BOARD::NAME) ptr = &MOTHER_BOARD::NAME;\
	};

namespace Sakura::refl
{
	// Object is a class similar in nature to std::any, only it does not require
	// C++17 and does not require stored objects to be copyable.

	inline void NoOp(void*) {}

	namespace detail
	{
		template <typename Fn, typename Tuple>
		inline constexpr void ForEachTuple(Tuple&& tuple, Fn&& fn)
		{
			hana::for_each(std::forward<Tuple>(tuple), fn);
		}

		template<class ...Fs> struct overload_set;
		template<class F1, class... Fs>
		struct overload_set<F1, Fs...> : F1, overload_set<Fs...>::type {
			using type = overload_set;

			overload_set(F1 head, Fs... tail)
				: F1(head), overload_set<Fs...>::type(tail...)
			{ }

			using F1::operator();
			using overload_set<Fs...>::type::operator();
		};

		template<class F>
		struct overload_set<F> : F {
			using type = F;
			using F::operator();
		};

		template<class... Fs>
		struct overload_visitor : overload_set<Fs...>::type {
			using type = overload_visitor;
			using overload_set<Fs...>::type::operator();

			overload_visitor(Fs... funcs)
				: overload_set<Fs...>::type(funcs...) {
			}
		};

		template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
		template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

		int constexpr length(const char* str)
		{
			return *str ? 1 + length(str + 1) : 0;
		}

#if true
		inline constexpr static const size_t _FNV_offset_basis = 14695981039346656037ULL;
		inline constexpr static const size_t _FNV_prime = 1099511628211ULL;
#else 
		inline constexpr static const size_t _FNV_offset_basis = 2166136261U;
		inline constexpr static const size_t _FNV_prime = 16777619U;
#endif 

		inline static const constexpr size_t _Fnv1a_append_bytes(size_t _Val, const char* _First,
			const size_t _Count) noexcept
		{
			// accumulate range [_First, _First + _Count) into partial FNV-1a hash _Val
			for (size_t _Idx = 0; _Idx < _Count; ++_Idx)
			{
				_Val ^= static_cast<size_t>(_First[_Idx]);
				_Val *= _FNV_prime;
			}
			return _Val;
		}

		template<typename T, std::size_t N>
		constexpr std::size_t arraySize(T(&)[N]) noexcept
		{
			return N;
		}

		constexpr std::size_t arraySize(std::nullptr_t nul) noexcept
		{
			return 0;
		}
	}


	struct Meta
	{
		struct MetaPiece
		{
			constexpr MetaPiece(const char* _t, const char* _v)
				: title(_t), value(_v)
			{}
			const char* title = nullptr;
			const char* value = nullptr;
		};
		constexpr Meta(const MetaPiece* _ms, uint32_t _mc)
			:metas(_ms), metaCount(_mc)
		{

		}
		template<int N>
		constexpr Meta(const MetaPiece(&_ms)[N])
			: metas(_ms), metaCount(N)
		{

		}
		constexpr Meta(const std::nullptr_t npt)
		{

		}
		const MetaPiece* metas = nullptr;//8 36
		uint32_t metaCount = 0;//4 40
	};

	struct Field
	{
		const char* name;//8
		const char* type;//8 16
		uint32_t offset;// 4 24
		Meta metas;
	};
	using Parameter = Field;

	// Reference is a non-const, type erased wrapper around any object.
	class Reference final
	{
	public:
		template<typename T>
		constexpr Reference(T& t);

		constexpr Reference();

		Reference(const Reference& o) = default;
		Reference& operator=(const Reference& o) = default;

		template <typename T>
		bool IsT() const;

		template <typename T>
		T& GetT() const
		{
			return *static_cast<T*>(data_);
		}
		size_t id_;
	private:
		void* data_ = nullptr;
	};

	// Dynamic Part
	template<typename T>
	inline static const Reference GetFieldT(const Reference& o, const std::string& fieldname){assert(0); return Reference();};

	template<typename T,
		std::enable_if_t<!std::is_pointer_v<T>, int> = 0>
	inline static const bool constexpr isAtomic(){ return false; }
	template<typename T,
		std::enable_if_t<std::is_pointer_v<T>, int> = 0>
	inline static const bool constexpr isAtomic() { return true; }

	template<typename T>
	struct ClassInfo
	{
		inline static const constexpr char* GetClassName() { return "NULL"; }
		inline static constexpr const auto all_methods() { return hana::make_tuple(); }
		inline static constexpr const auto all_fields() { return hana::make_tuple(); }
		inline static constexpr const auto all_static_fields() { return hana::make_tuple(); }
		inline static constexpr const auto all_static_methods() { return hana::make_tuple(); }
	};
	template<typename T>
	struct EnumInfo
	{
		inline static const constexpr char* GetEnumName() { return "NULL"; }
		//inline static const constexpr Meta::MetaPiece meta = {...};
		//inline static const constexpr Meta::MetaPiece meta_EONE = {...}; 
		//inline static constexpr Meta perMeta[n] = {...};
	};

	using namespace std;
#define GEN_REFL_BASIC_TYPES_TWO_PARAM(T, NAME) \
	template<> inline const Reference GetFieldT<T>(const Reference& o, const std::string& fieldName){return o;}\
	template<> inline const constexpr bool isAtomic<T>(){return true;}\
	template<> struct ClassInfo<T>\
	{\
		inline static const constexpr char* GetClassName(){return #NAME;}\
		inline static constexpr const auto all_fields() { return hana::make_tuple(); }\
		inline static constexpr const auto all_static_fields() { return hana::make_tuple(); }\
		inline static constexpr const auto all_methods() { return hana::make_tuple(); }\
		inline static constexpr const auto all_static_methods() { return hana::make_tuple(); }\
		inline static const constexpr Meta::MetaPiece meta[1] =\
		{\
			{"description", "Class of "#T}\
		};\
	};


#define GEN_REFL_BASIC_TYPES(T) GEN_REFL_BASIC_TYPES_TWO_PARAM(T, T)

	GEN_REFL_BASIC_TYPES(void);
	GEN_REFL_BASIC_TYPES(char);
	GEN_REFL_BASIC_TYPES(uint8_t);
	GEN_REFL_BASIC_TYPES(uint16_t);
	GEN_REFL_BASIC_TYPES(uint32_t);
	GEN_REFL_BASIC_TYPES(uint64_t);
	GEN_REFL_BASIC_TYPES(int8_t);
	GEN_REFL_BASIC_TYPES(int16_t);
	GEN_REFL_BASIC_TYPES(int32_t);
	GEN_REFL_BASIC_TYPES(int64_t);
	GEN_REFL_BASIC_TYPES(float);
	GEN_REFL_BASIC_TYPES(double);
	GEN_REFL_BASIC_TYPES_TWO_PARAM(string, string);
	GEN_REFL_BASIC_TYPES_TWO_PARAM(pmr::string, string);
	GEN_REFL_BASIC_TYPES_TWO_PARAM(std::byte, byte);
	GEN_REFL_BASIC_TYPES_TWO_PARAM(const char*, const char*);


	template<> struct ClassInfo<Field>
	{
		inline static const constexpr char* GetClassName() { return "Field"; }
		inline static const constexpr Meta::MetaPiece meta[1] =
		{
			{"description", "Field Informations."}
		};
		inline static constexpr const auto all_fields()
		{
			SFIELD_INFO(name, const char*, Field, nullptr)
			SFIELD_INFO(type, const char*, Field, nullptr)
			SFIELD_INFO(offset, uint32_t, Field, nullptr)
			SFIELD_INFO(metas, meta, Field, nullptr)
			return hana::make_tuple(name_info(), type_info(), offset_info(), metas_info());
		}
	};

	template<> inline const Reference GetFieldT<Field>(const Reference& o, const std::string& name)
	{
		if (name == "name")
			return o.GetT<Field>().name;
		else if (name == "type")
			return o.GetT<Field>().type;
		else if (name == "offset")
			return o.GetT<Field>().offset;
		else if (name == "metas")
			return o.GetT<Field>().metas;
		assert(0 && "No field of this name.");
		return o;
	}

	template<typename T>
	struct SClass
	{
		using ClassName = std::decay_t<T>;
		using info = ClassInfo<ClassName>;
		inline static const constexpr char* GetName() { return info::GetClassName(); }
		inline static const constexpr Meta GetClassMeta() { return Meta(info::meta); }
		inline static const constexpr std::size_t id_ =
			detail::_Fnv1a_append_bytes(Sakura::refl::detail::_FNV_offset_basis,
				info::GetClassName(), detail::length(info::GetClassName()) * sizeof(char));
		inline static const constexpr std::size_t GetTypeId() { return id_; }
		inline static const constexpr std::size_t GetStaticFieldCount() noexcept
		{
			return 0;
		}
		inline static const Reference GetStaticField(const Reference& o, const std::string& fieldName)
		{
			return Reference();
		}
		inline static const Field GetFieldMeta(const std::string& name) noexcept
		{
			for (auto i = 0; i < GetFieldCount(); i++)
			{
				if (name == info::fields[i].name)
					return info::fields[i];
			}
			const char* _n = name.c_str();
			assert(0 && "No Field named" && _n);
			return info::fields[999999999];
		}

		inline static const constexpr std::size_t GetFieldCount() noexcept
		{
			return detail::arraySize(info::fields);
		}

		template <bool atomic, typename Fn, typename Tuple>
		inline static constexpr void __for_each_field_meta_impl(Tuple&& tup, Fn&& fn)
		{
			detail::ForEachTuple(std::forward<Tuple>(tup),
				[&fn](auto&& field_schema)
				{
						fn(field_schema.fd);
				});
		}

		template <bool atomic, typename V, typename Fn, typename Tuple>
		inline static constexpr void __for_each_field_impl(Tuple&& tup, V&& value, Fn&& fn)
		{
			detail::ForEachTuple(std::forward<Tuple>(tup),
				[&value, &fn](auto&& field_schema)
				{
					using ClassName_C = std::decay_t<decltype(value.*(field_schema.ptr))>;
					if constexpr (!isAtomic<ClassName_C>() && atomic)
					{
						SClass<ClassName_C>::ForEachFieldAtomic(std::forward<ClassName_C>(value.*(field_schema.ptr)), fn);
						constexpr int leng = hana::length(ClassInfo<ClassName_C>::all_static_fields());
						if constexpr (leng > 0)
							SClass<ClassName_C>::ForEachStaticFieldAtomic(fn);
					}
					else
					{
						fn(value.*(field_schema.ptr), field_schema.fd);
					}
				});
		}

		template <bool atomic, typename Fn, typename Tuple>
		inline static constexpr void __for_each_static_field_impl(Tuple&& tup, Fn&& fn)
		{
			detail::ForEachTuple(std::forward<Tuple>(tup),
				[&fn](auto&& field_schema)
				{
					using ClassName_CS = std::decay_t<decltype(*(field_schema.ptr))>;
					if constexpr (!isAtomic<ClassName_CS>() && atomic)
						SClass<ClassName_CS>::ForEachFieldAtomic(*(field_schema.ptr), fn);
					else
					{
						fn(*field_schema.ptr, field_schema.fd);
					}
				});
		}

		template <typename Fn>
		inline static constexpr void ForEachFieldMeta(Fn&& fn)
		{
			__for_each_field_meta_impl<false>(info::all_fields(), fn);
		}
		template <typename V, typename Fn>
		inline static constexpr void ForEachField(V&& value, Fn&& fn)
		{
			__for_each_field_impl<false>(info::all_fields(), value, fn);
		}
		template <typename V, typename Fn>
		inline static constexpr void ForEachFieldAtomic(V&& value, Fn&& fn)
		{
			__for_each_field_impl<true>(info::all_fields(), value, fn);
		}

		template <typename Fn>
		inline static constexpr void ForEachStaticFieldMeta(Fn&& fn)
		{
			__for_each_field_meta_impl<false>(info::all_static_fields(), fn);
		}
		template <typename Fn>
		inline static constexpr void ForEachStaticField(Fn&& fn)
		{
			__for_each_static_field_impl<false>(info::all_static_fields(), fn);
		}
		template <typename Fn>
		inline static constexpr void ForEachStaticFieldAtomic(Fn&& fn)
		{
			__for_each_static_field_impl<true>(info::all_static_fields(), fn);
		}

		template <typename Fn>
		inline static constexpr void ForEachMethodMeta(Fn&& fn)
		{
			__for_each_field_meta_impl<false>(info::all_methods(), fn);
		}
		template <typename V, typename Fn>
		inline static constexpr void ForEachMethod(V&& value, Fn&& fn)
		{
			detail::ForEachTuple(info::all_methods(),
				[&value, &fn](auto&& field_schema)
				{
					fn(field_schema.ptr, field_schema.fd);
				});
		}

		template <typename Fn>
		inline static constexpr void ForEachStaticMethodMeta(Fn&& fn)
		{
			__for_each_field_meta_impl<false>(info::all_static_methods(), fn);
		}
		template <typename Fn>
		inline static constexpr void ForEachStaticMethod(Fn&& fn)
		{
			detail::ForEachTuple(info::all_static_methods(),
				[&fn](auto&& field_schema)
				{
					fn(field_schema.ptr, field_schema.fd);
				});
		}


		// Dynamic Part
		inline static const Reference GetField(const Reference& o, const std::string& fieldName)
		{
			return GetFieldT<ClassName>(o, fieldName);
		}
/*
		template <typename T>
		T& GetT() const
		{
			return *static_cast<T*>(data_);
		}
*/
		template<typename TT>
		inline static const TT GetField(const Reference& o, const std::string& fieldName)
		{
			return *static_cast<TT*>(GetFieldT<ClassName>(o, fieldName).data_);
		}
	};

	template <typename T>
	auto constexpr GetTypeId()
	{
		return SClass<T>::GetTypeId();
	}






	// Dynamic Part

	class Object final
	{
	public:
		Object();
		template<typename T>
		explicit Object(T&& t);

		Object(Object&& o) noexcept
			: deleter_(NoOp)
		{
			*this = std::move(o);
		}
		Object& operator=(Object&& o) noexcept
		{
			// Release existing resource if any.
			deleter_(data_);

			id_ = o.id_;
			data_ = o.data_;
			deleter_ = move(o.deleter_);
			o.deleter_ = NoOp;
			return *this;
		}
		Object(const Object& o) = delete;
		Object& operator=(const Object& o) = delete;

		~Object()
		{
			deleter_(data_);
		}

		template<typename T>
		bool IsT() const;

		template<typename T>
		const T& GetT() const
		{
			return *static_cast<T*>(data_);
		}
		bool IsVoid() const;
	private:
		size_t id_;
		void* data_ = nullptr;
		std::function<void(void*)> deleter_;
	};



	struct IType
	{
		virtual ~IType() = default;
		virtual const char* GetName() const = 0;
	};

	enum EFlag : uint32_t
	{
		EFunction = 1,
		EMethod = EFunction << 1,
		EAtomic = EMethod << 1,
		EStruct = EAtomic << 1,
		EClass = EStruct << 1,
		EEnum = EClass << 1,
		EConst = EEnum << 1,
		EPtr = EConst << 1,
		EReference = EPtr << 1,
		ETemplate = EReference << 1
	};
	using EFlags = uint32_t;



	template <typename T>
	inline Object::Object(T&& t)
		: id_(GetTypeId<std::decay_t<T>>()),
		data_(new std::decay_t<T>(std::forward<T>(t)))
	{
		// This is not part of the initializer list because it
		// doesn't compile on VC.
		deleter_ = [](void* data)
		{
			delete static_cast<std::decay_t<T>*>(data);
		};
	}

	inline Object::Object()
		: id_(SClass<void>::GetTypeId())
		, deleter_(NoOp)
	{

	}

	template<typename T>
	inline bool Object::IsT() const
	{
		return (SClass<T>::GetTypeId() == id_);
	}

	template<typename T>
	inline constexpr Reference::Reference(T& t)
		: id_(GetTypeId<std::remove_reference_t<T>>())
		, data_((void*)&t)
	{

	}

	inline constexpr Reference::Reference()
		: id_(GetTypeId<std::nullptr_t>()), data_(nullptr)
	{
			
	}

	template <typename T>
	inline bool Reference::IsT() const
	{
		return (SClass<T>::GetTypeId() == id_);
	}

	struct IFunction : public IType
	{
		virtual int GetParameterCount() const = 0;
		virtual Parameter GetReturnType() const = 0;
		virtual Parameter GetParameter(int i) const = 0;

		// Syntactic sugar for calling Invoke().
		template <typename... Ts>
		Object operator()(Ts&&... ts);

		virtual Object Invoke(const std::vector<Object>& args) = 0;
	};

	class IMethod : public IType
	{
	public:
		virtual int GetParameterCount() const = 0;
		virtual Parameter GetReturnType() const = 0;
		virtual Parameter GetParameter(int i) const = 0;

		// Syntactic sugar for calling Invoke().
		template <typename... Ts>
		Object operator()(const Reference& o, Ts&&... ts);

		virtual Object Invoke(
			const Reference& o, const std::vector<Object>& args) = 0;
	};

	class IClass : public IType
	{
	public:
		virtual int GetFieldCount() const = 0;
		virtual Reference GetField(
			const Reference& o, const std::string& name) const = 0;

		virtual int GetStaticFieldCount() const = 0;
		virtual Reference GetStaticField(const std::string& name) const = 0;

		virtual int GetMethodCount() const = 0;
		virtual std::vector<std::unique_ptr<IMethod>> GetMethod(
			const std::string& name) const = 0;

		virtual int GetStaticMethodCount() const = 0;
		virtual std::vector<std::unique_ptr<IFunction>> GetStaticMethod(
			const std::string& name) const = 0;
	};

	class IEnum : public IType
	{
	public:
		virtual std::vector<std::string> GetStringValues() const = 0;
		virtual std::vector<int> GetIntValues() const = 0;
		virtual bool TryTranslate(const std::string& value, int& out) = 0;
		virtual bool TryTranslate(int value, std::string& out) = 0;
	};

	template<typename T>
	struct DynSClass : IClass
	{
		virtual const char* GetName() const override final { return GetClassName<T>(); }
	};

	template<typename T> struct SEnum : IEnum {};
	template <typename T, T t> struct Function : IFunction {};
	template <typename T, T t> struct Method : IMethod {};
}

namespace Sakura
{
	template<class... Fs>
	const typename refl::detail::overload_visitor<Fs...>::type overload(Fs... x)
	{
		return refl::detail::overload_visitor<Fs...>(x...);
	}
}
