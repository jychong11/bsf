#pragma once

#include "CmPrerequisites.h"
#include "CmAsyncOp.h"
#include <boost/preprocessor.hpp>

namespace BansheeEngine
{
	/**
	 * @brief	This class provides some common functionality that all low-level objects
	 * 			used on the core thread need to implement.
	 * 			
	 * @note	This involves initializing, keeping track of, and releasing GPU resources.
	 * 			All core GPU objects are initialized on the core thread, and destroyed on the core thread,
	 * 			so majority of these methods will just schedule object initialization/destruction.
	 * 			Non-GPU core objects can normally be initialized on the caller thread.
	 */
	class CM_EXPORT CoreObject
	{
	protected:
		/**
		 * @brief	Values that represent current state of the object
		 */
		enum Flags
		{
			CGO_INITIALIZED = 0x01, /**< Object has been fully initialized and may be used. */
			CGO_INIT_ON_CORE_THREAD = 0x02, /**< Object requires initialization on core thread. */
			CGO_SCHEDULED_FOR_INIT = 0x04, /**< Object has been scheduled for initialization but core thread has not completed it yet. */
			CGO_SCHEDULED_FOR_DELETE = 0x08 /**< Object has been scheduled for deletion but core thread has not completed it yet. */
		};

	public:
		/**
		 * @brief	Constructs a new core object.
		 *
		 * @param	requiresGpuInit	(optional) If true the objects initialize_internal and destroy_internal methods
		 * 							will be called from the core thread asynchronously. Otherwise they will be called 
		 * 							by the caller thread synchronously.
		 */
		CoreObject(bool requiresGpuInit = true);
		virtual ~CoreObject();

		/**
		 * @brief	Destroys all GPU resources of this object.
		 * 			
		 * @note	If is created with "CGO_INIT_ON_CORE_THREAD" flag destruction is not done immediately, 
		 * 			and is instead just scheduled on the core thread. 
		 * 			Unless called from core thread in which case it is executed immediately.
		 * 			Objects without "CGO_INIT_ON_CORE_THREAD" flag are destructed immediately.
		 */
		virtual void destroy();

		/**
		 * @brief	Initializes all the internal resources of this object. Should be called by the
		 * 			factory creation methods automatically after construction and not by user directly.
		 * 					
		 * @note	If is created with "CGO_INIT_ON_CORE_THREAD" flag initialization is not done immediately, 
		 * 			and is instead just scheduled on the core thread. 
		 * 			Unless called from core thread in which case it is executed immediately.
		 * 			Objects without "CGO_INIT_ON_CORE_THREAD" flag are initialized immediately.
		 */
		virtual void initialize();

		/**
		 * @brief	Returns true if the object has been properly initialized. You are not
		 * 			allowed to call any methods on the resource until you are sure resource is initialized.
		 * 			
		 * @note	Normally CPU objects are initialized on creation and this will never be false, and GPU
		 * 			objects are initialized when the core thread processes them.
		 */
		bool isInitialized() const { return (mFlags & CGO_INITIALIZED) != 0; }

		/**
		 * @brief	Blocks the current thread until the resource is fully initialized.
		 * 			
		 * @note	If you call this without calling initialize first a deadlock will occur.
		 * 			You should not call this from core thread.
		 */
		void synchronize();

		/**
		 * @brief	Internal method. Sets a shared this pointer to this object. This MUST be called immediately after construction.
		 *
		 * @note	Called automatically by the factory creation methods so user should not call this manually.
		 */
		void _setThisPtr(std::shared_ptr<CoreObject> ptrThis);

		/**
		 * @brief	Returns an unique identifier for this object.
		 */
		UINT64 getInternalID() const { return mInternalID; }

		/**
		 * @brief	Internal method. Schedules the object to be destroyed, and then deleted.
		 */
		template<class T, class MemAlloc>
		static void _deleteDelayed(CoreObject* obj)
		{
			_deleteDelayedInternal(obj);

			if(obj->isInitialized())
			{
				std::shared_ptr<CoreObject> thisPtr(obj);
				obj->_setThisPtr(thisPtr);
				obj->destroy();
			}
			else
			{
				cm_delete<MemAlloc, T>((T*)obj);
			}
		}

		/**
		 * @brief	Returns a shared_ptr version of "this" pointer.
		 */
		std::shared_ptr<CoreObject> getThisPtr() const { return mThis.lock(); }

	protected:
		/**
		 * @brief	Frees all of the objects dynamically allocated memory. All derived classes that have something to free
		 * 			should do it here instead of their destructor. All derived classes need to call this base method when they're done.
		 * 			
		 * @note	For objects with "CGO_INIT_ON_CORE_THREAD" flag this is scheduled to be executed on the core thread, 
		 * 			so normally you want to destroy all GPU specific resources here.
		 */
		virtual void destroy_internal();

		/**
		 * @brief	Initializes all the internal resources of this object. Needs to be called before doing
		 * 			any operations with the object. All derived classes also need to call this base method.
		 * 			
		 * @note	For objects with "CGO_INIT_ON_CORE_THREAD" flag this is scheduled to be executed on the core thread, 
		 * 			so normally you want to initialize all GPU specific resources here.
		 */
		virtual void initialize_internal();

		/**
		 * @brief	Performs some internal checks when an object is being deleted.
		 */
		static void _deleteDelayedInternal(CoreObject* obj);

		/**
		 * @brief	Queues a command to be executed on the core thread, without a return value.
		 * 			
		 * @note	Requires a shared pointer to the object this function will be executed on, in order to 
		 * 			make sure the object is not deleted before the command executes. Can be null if the 
		 * 			function is static or global.
		 */
		static void queueGpuCommand(std::shared_ptr<CoreObject>& obj, std::function<void()> func);

		/**
		 * @brief	Queues a command to be executed on the core thread, with a return value in the form of AsyncOp.
		 * 			
		 * @see		AsyncOp
		 * 			
		 * @note	Requires a shared pointer to the object this function will be executed on, in order to
		 * 			make sure the object is not deleted before the command executes. Can be null if the
		 * 			function is static or global.
		 */
		static AsyncOp queueReturnGpuCommand(std::shared_ptr<CoreObject>& obj, std::function<void(AsyncOp&)> func);

		bool isScheduledToBeInitialized() const { return (mFlags & CGO_SCHEDULED_FOR_INIT) != 0; }
		bool isScheduledToBeDeleted() const { return (mFlags & CGO_SCHEDULED_FOR_DELETE) != 0; }
		bool requiresInitOnCoreThread() const { return (mFlags & CGO_INIT_ON_CORE_THREAD) != 0; }

		void setIsInitialized(bool initialized) { mFlags = initialized ? mFlags | CGO_INITIALIZED : mFlags & ~CGO_INITIALIZED; }
		void setScheduledToBeInitialized(bool scheduled) { mFlags = scheduled ? mFlags | CGO_SCHEDULED_FOR_INIT : mFlags & ~CGO_SCHEDULED_FOR_INIT; }
		void setScheduledToBeDeleted(bool scheduled) { mFlags = scheduled ? mFlags | CGO_SCHEDULED_FOR_DELETE : mFlags & ~CGO_SCHEDULED_FOR_DELETE; }
	private:
		friend class CoreObjectManager;

		volatile UINT8 mFlags;
		UINT64 mInternalID; // ID == 0 is not a valid ID
		std::weak_ptr<CoreObject> mThis;

		CM_STATIC_THREAD_SYNCHRONISER(mCoreGpuObjectLoadedCondition)
		CM_STATIC_MUTEX(mCoreGpuObjectLoadedMutex)

		/**
		 * @brief	Queues object initialization command on the core thread. The command is added to the
		 * 			primary core thread queue and will be executed as soon as the core thread is ready.
		 */
		static void queueInitializeGpuCommand(std::shared_ptr<CoreObject>& obj);

		/**
		 * @brief	Queues object destruction command on the core thread. The command is added to the
		 * 			core thread accessor of this thread and will be executed after accessor commands
		 * 			are submitted and any previously queued commands are executed.
		 *
		 * @note	It is up to the caller to ensure no other accessors attempt to use this object.
		 */
		static void queueDestroyGpuCommand(std::shared_ptr<CoreObject>& obj);

		/**
		 * @brief	Helper wrapper method used for queuing commands with no return value on the core thread.
		 */
		static void executeGpuCommand(std::shared_ptr<CoreObject>& obj, std::function<void()> func);

		/**
		 * @brief	Helper wrapper method used for queuing commands with a return value on the core thread.
		 */
		static void executeReturnGpuCommand(std::shared_ptr<CoreObject>& obj, std::function<void(AsyncOp&)> func, AsyncOp& op); 
	};

	/**
	 * @brief	Creates a new core object using the specified allocators and returns a shared pointer to it.
	 *
	 * @note	All core thread object shared pointers must be created using this method or its overloads
	 * 			and you should not create them manually.
	 */
#define MAKE_CM_NEW_CORE(z, n, unused)                                     \
	template<class Type, class MainAlloc, class PtrDataAlloc BOOST_PP_ENUM_TRAILING_PARAMS(n, class T)>     \
	std::shared_ptr<Type> cm_core_ptr(BOOST_PP_ENUM_BINARY_PARAMS(n, T, t) ) { \
	return std::shared_ptr<Type>(cm_new<Type, MainAlloc>(BOOST_PP_ENUM_PARAMS (n, t)), &CoreObject::_deleteDelayed<Type, MainAlloc>, StdAlloc<PtrDataAlloc>());     \
	}

	BOOST_PP_REPEAT(15, MAKE_CM_NEW_CORE, ~)

#undef MAKE_CM_NEW_CORE

	/**
	 * @brief	Creates a new core object using the specified allocator and returns a shared pointer to it.
	 *
	 * @note	All core thread object shared pointers must be created using this method or its overloads
	 * 			and you should not create them manually.
	 */
#define MAKE_CM_NEW_CORE(z, n, unused)                                     \
	template<class Type, class MainAlloc BOOST_PP_ENUM_TRAILING_PARAMS(n, class T)>     \
	std::shared_ptr<Type> cm_core_ptr(BOOST_PP_ENUM_BINARY_PARAMS(n, T, t) ) { \
	return std::shared_ptr<Type>(cm_new<Type, MainAlloc>(BOOST_PP_ENUM_PARAMS (n, t)), &CoreObject::_deleteDelayed<Type, MainAlloc>, StdAlloc<GenAlloc>());     \
	}

	BOOST_PP_REPEAT(15, MAKE_CM_NEW_CORE, ~)

#undef MAKE_CM_NEW_CORE

	/**
	 * @brief	Creates a new core object and returns a shared pointer to it.
	 *
	 * @note	All core thread object shared pointers must be created using this method or its overloads
	 * 			and you should not create them manually.
	 */
#define MAKE_CM_NEW_CORE(z, n, unused)                                     \
	template<class Type BOOST_PP_ENUM_TRAILING_PARAMS(n, class T)>     \
	std::shared_ptr<Type> cm_core_ptr(BOOST_PP_ENUM_BINARY_PARAMS(n, T, t) ) { \
	return std::shared_ptr<Type>(cm_new<Type, GenAlloc>(BOOST_PP_ENUM_PARAMS (n, t)), &CoreObject::_deleteDelayed<Type, GenAlloc>, StdAlloc<GenAlloc>());     \
	}

	BOOST_PP_REPEAT(15, MAKE_CM_NEW_CORE, ~)

#undef MAKE_CM_NEW_CORE

	/**
	 * @brief	Creates a core object shared pointer using a previously constructed object.
	 *
	 * @note	All core thread object shared pointers must be created using this method or its overloads
	 * 			and you should not create them manually.
	 */
	template<class Type, class MainAlloc>
	std::shared_ptr<Type> cm_core_ptr(Type* data) 
	{
		return std::shared_ptr<Type>(data, &CoreObject::_deleteDelayed<Type, MainAlloc>, StdAlloc<GenAlloc>());  
	}

	/**
	 * @brief	Creates a core object shared pointer using a previously constructed object.
	 *
	 * @note	All core thread object shared pointers must be created using this method or its overloads
	 * 			and you should not create them manually.
	 */
	template<class Type, class MainAlloc, class PtrDataAlloc>
	std::shared_ptr<Type> cm_core_ptr(Type* data) 
	{
		return std::shared_ptr<Type>(data, &CoreObject::_deleteDelayed<Type, MainAlloc>, StdAlloc<PtrDataAlloc>());  
	}
}