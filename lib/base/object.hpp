/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef OBJECT_H
#define OBJECT_H

#include "base/i2-base.hpp"
#include "base/debug.hpp"
#include <boost/thread/thread.hpp>

#ifndef _DEBUG
#include <boost/thread/mutex.hpp>
#else /* _DEBUG */
#include <boost/thread/recursive_mutex.hpp>
#endif /* _DEBUG */

#ifndef _MSC_VER
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/smart_ptr/make_shared.hpp>

using boost::shared_ptr;
using boost::weak_ptr;
using boost::enable_shared_from_this;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;
using boost::make_shared;
#else /* _MSC_VER */
#include <memory>

using std::shared_ptr;
using std::weak_ptr;
using std::enable_shared_from_this;
using std::dynamic_pointer_cast;
using std::static_pointer_cast;
using std::make_shared;
#endif /* _MSC_VER */

#include <boost/tuple/tuple.hpp>
using boost::tie;

namespace icinga
{

class Value;
class Object;
class Type;

#define DECLARE_PTR_TYPEDEFS(klass) \
	typedef shared_ptr<klass> Ptr; \
	typedef weak_ptr<klass> WeakPtr

#define IMPL_TYPE_LOOKUP(klass) \
	static shared_ptr<Type> TypeInstance; \
	inline virtual shared_ptr<Type> GetReflectionType(void) const \
	{ \
		return TypeInstance; \
	}

#define DECLARE_OBJECT(klass) \
	DECLARE_PTR_TYPEDEFS(klass); \
	IMPL_TYPE_LOOKUP(klass);

template<typename T>
shared_ptr<Object> DefaultObjectFactory(void)
{
	return make_shared<T>();
}

typedef shared_ptr<Object> (*ObjectFactory)(void);

template<typename T>
struct TypeHelper
{
	static ObjectFactory GetFactory(void)
	{
		return DefaultObjectFactory<T>;
	}
};

/**
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 *
 * @ingroup base
 */
class I2_BASE_API Object : public enable_shared_from_this<Object>
{
public:
	DECLARE_OBJECT(Object);

	Object(void);
	virtual ~Object(void);

	virtual void SetField(int id, const Value& value);
	virtual Value GetField(int id) const;

	/**
	 * Holds a shared pointer and provides support for implicit upcasts.
	 *
	 * @ingroup base
	 */
	class SharedPtrHolder
	{
	public:
		/**
		 * Constructor for the SharedPtrHolder class.
		 *
		 * @param object The shared pointer that should be used to
		 *		 construct this shared pointer holder.
		 */
		explicit SharedPtrHolder(const Object::Ptr& object)
			: m_Object(object)
		{ }

		/**
		 * Retrieves a shared pointer for the object that is associated
		 * this holder instance.
		 *
		 * @returns A shared pointer.
		 */
		template<typename T>
		operator shared_ptr<T>(void) const
		{
#ifdef _DEBUG
			shared_ptr<T> other = dynamic_pointer_cast<T>(m_Object);
			ASSERT(other);
#else /* _DEBUG */
			shared_ptr<T> other = static_pointer_cast<T>(m_Object);
#endif /* _DEBUG */

			return other;
		}

		/**
		 * Retrieves a weak pointer for the object that is associated
		 * with this holder instance.
		 *
		 * @returns A weak pointer.
		 */
		template<typename T>
		operator weak_ptr<T>(void) const
		{
			return static_cast<shared_ptr<T> >(*this);
		}

		operator Value(void) const;

	private:
		Object::Ptr m_Object; /**< The object that belongs to this
					   holder instance */
	};

#ifdef _DEBUG
	bool OwnsLock(void) const;
#endif /* _DEBUG */

protected:
	SharedPtrHolder GetSelf(void);

private:
	Object(const Object& other);
	Object& operator=(const Object& rhs);

#ifndef _DEBUG
	typedef boost::mutex MutexType;
#else /* _DEBUG */
	typedef boost::recursive_mutex MutexType;

	static boost::mutex m_DebugMutex;
	mutable bool m_Locked;
	mutable boost::thread::id m_LockOwner;
#endif /* _DEBUG */

	mutable MutexType m_Mutex;

	friend struct ObjectLock;
};

/**
 * Compares a weak pointer with a raw pointer.
 *
 * @ingroup base
 */
template<class T>
struct WeakPtrEqual
{
private:
	const void *m_Ref; /**< The object. */

public:
	/**
	 * Constructor for the WeakPtrEqual class.
	 *
	 * @param ref The object that should be compared with the weak pointer.
	 */
	WeakPtrEqual(const void *ref) : m_Ref(ref) { }

	/**
	 * Compares the two pointers.
	 *
	 * @param wref The weak pointer.
	 * @returns true if the pointers point to the same object, false otherwise.
	 */
	bool operator()(const weak_ptr<T>& wref) const
	{
		return (wref.lock().get() == static_cast<const T *>(m_Ref));
	}
};

template<typename T>
class ObjectImpl
{
};

}

#endif /* OBJECT_H */
