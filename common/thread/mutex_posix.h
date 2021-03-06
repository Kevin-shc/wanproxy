/*
 * Copyright (c) 2010-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	COMMON_THREAD_MUTEX_POSIX_H
#define	COMMON_THREAD_MUTEX_POSIX_H

struct MutexState {
	pthread_mutex_t mutex_;
	pthread_mutexattr_t mutex_attr_;
#ifndef NDEBUG
	pthread_cond_t cond_;
	Thread::ID owner_;
	std::deque<Thread::ID> waiters_;
#endif

	MutexState(void)
	: mutex_(),
	  mutex_attr_()
#ifndef NDEBUG
	, cond_(),
	  owner_(NULL),
	  waiters_()
#endif
	{
		int error;

		error = pthread_mutexattr_init(&mutex_attr_);
		ASSERT_ZERO("/mutex/posix/state", error);

		error = pthread_mutexattr_settype(&mutex_attr_, PTHREAD_MUTEX_RECURSIVE);
		ASSERT_ZERO("/mutex/posix/state", error);

		error = pthread_mutex_init(&mutex_, &mutex_attr_);
		ASSERT_ZERO("/mutex/posix/state", error);

#ifndef NDEBUG
		error = pthread_cond_init(&cond_, NULL);
#endif
	}

	~MutexState()
	{
		int error;

		error = pthread_mutex_destroy(&mutex_);
		ASSERT_ZERO("/mutex/posix/state", error);

		error = pthread_mutexattr_destroy(&mutex_attr_);
		ASSERT_ZERO("/mutex/posix/state", error);

#ifndef NDEBUG
		error = pthread_cond_destroy(&cond_);
		ASSERT_ZERO("/mutex/posix/state", error);
#endif

#if 0 /* XXX What about extern Mutexes?  */
		Thread::ID self = Thread::selfID();
		ASSERT_NON_NULL("/mutex/posix/state", self);
		ASSERT("/mutex/posix/state", owner_ == self);
#endif
	}

	/*
	 * Acquire the underlying lock.
	 */
	void lock(void)
	{
		int error;

		error = pthread_mutex_lock(&mutex_);
		ASSERT_ZERO("/mutex/posix/state", error);
	}

	/*
	 * Acquire the underlying lock without blocking.
	 */
	bool try_lock(void)
	{
		int error;

		error = pthread_mutex_trylock(&mutex_);
		if (error == 0)
			return (true);
		ASSERT_EQUAL("/mutex/posix/state", error, EBUSY);
		return (false);
	}

#ifndef NDEBUG
	/*
	 * Acquire ownership of this mutex.
	 */
	void lock_acquire(void)
	{
		int error;

		Thread::ID self = Thread::selfID();
		ASSERT_NON_NULL("/mutex/posix/state", self);

		if (owner_ != NULL) {
			waiters_.push_back(self);
			for (;;) {
				error = pthread_cond_wait(&cond_, &mutex_);
				ASSERT_ZERO("/mutex/posix/state", error);
				if (owner_ != NULL)
					continue;
				if (waiters_.front() != self)
					continue;
				waiters_.pop_front();
				break;
			}
		}
		owner_ = self;
	}

	/*
	 * Acquire ownership of this mutex without blocking.
	 */
	bool lock_acquire_try(void)
	{
		if (owner_ != NULL)
			return (false);

		Thread::ID self = Thread::selfID();
		ASSERT_NON_NULL("/mutex/posix/state", self);
		owner_ = self;
		return (true);
	}
#endif

	/*
	 * Release the underlying lock.
	 */
	void unlock(void)
	{
		int error;

		error = pthread_mutex_unlock(&mutex_);
		ASSERT_ZERO("/mutex/posix/state", error);
	}

#ifndef NDEBUG
	/*
	 * Release ownership of this Mutex.
	 */
	void lock_release(void)
	{
		int error;

		Thread::ID self = Thread::selfID();
		ASSERT_NON_NULL("/mutex/posix/state", self);

		if (owner_ == NULL) {
			HALT("/mutex/posix/state") << "Attempt to unlock already-unlocked mutex.";
			return;
		}
		ASSERT("/mutex/posix/state", owner_ == self);
		owner_ = NULL;

		if (waiters_.empty())
			return;

		error = pthread_cond_broadcast(&cond_);
		ASSERT_ZERO("/mutex/posix/state", error);
	}
#endif
};

#endif /* !COMMON_THREAD_MUTEX_POSIX_H */
