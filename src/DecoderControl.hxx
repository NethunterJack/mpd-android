/*
 * Copyright (C) 2003-2013 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_DECODER_CONTROL_HXX
#define MPD_DECODER_CONTROL_HXX

#include "DecoderCommand.hxx"
#include "AudioFormat.hxx"
#include "thread/Mutex.hxx"
#include "thread/Cond.hxx"
#include "util/Error.hxx"

#include <assert.h>
#include <stdint.h>

/* damn you, windows.h! */
#ifdef ERROR
#undef ERROR
#endif

struct Song;
class MusicBuffer;
class MusicPipe;
typedef struct _GThread GThread;

enum class DecoderState : uint8_t {
	STOP = 0,
	START,
	DECODE,

	/**
	 * The last "START" command failed, because there was an I/O
	 * error or because no decoder was able to decode the file.
	 * This state will only come after START; once the state has
	 * turned to DECODE, by definition no such error can occur.
	 */
	ERROR,
};

struct decoder_control {
	/** the handle of the decoder thread, or NULL if the decoder
	    thread isn't running */
	GThread *thread;

	/**
	 * This lock protects #state and #command.
	 */
	mutable Mutex mutex;

	/**
	 * Trigger this object after you have modified #command.  This
	 * is also used by the decoder thread to notify the caller
	 * when it has finished a command.
	 */
	Cond cond;

	/**
	 * The trigger of this object's client.  It is signalled
	 * whenever an event occurs.
	 */
	Cond client_cond;

	DecoderState state;
	DecoderCommand command;

	/**
	 * The error that occurred in the decoder thread.  This
	 * attribute is only valid if #state is #DecoderState::ERROR.
	 * The object must be freed when this object transitions to
	 * any other state (usually #DecoderState::START).
	 */
	Error error;

	bool quit;
	bool seek_error;
	bool seekable;
	double seek_where;

	/** the format of the song file */
	AudioFormat in_audio_format;

	/** the format being sent to the music pipe */
	AudioFormat out_audio_format;

	/**
	 * The song currently being decoded.  This attribute is set by
	 * the player thread, when it sends the #DecoderCommand::START
	 * command.
	 *
	 * This is a duplicate, and must be freed when this attribute
	 * is cleared.
	 */
	Song *song;

	/**
	 * The initial seek position (in milliseconds), e.g. to the
	 * start of a sub-track described by a CUE file.
	 *
	 * This attribute is set by dc_start().
	 */
	unsigned start_ms;

	/**
	 * The decoder will stop when it reaches this position (in
	 * milliseconds).  0 means don't stop before the end of the
	 * file.
	 *
	 * This attribute is set by dc_start().
	 */
	unsigned end_ms;

	float total_time;

	/** the #music_chunk allocator */
	MusicBuffer *buffer;

	/**
	 * The destination pipe for decoded chunks.  The caller thread
	 * owns this object, and is responsible for freeing it.
	 */
	MusicPipe *pipe;

	float replay_gain_db;
	float replay_gain_prev_db;
	char *mixramp_start;
	char *mixramp_end;
	char *mixramp_prev_end;

	decoder_control();
	~decoder_control();

	/**
	 * Locks the object.
	 */
	void Lock() const {
		mutex.lock();
	}

	/**
	 * Unlocks the object.
	 */
	void Unlock() const {
		mutex.unlock();
	}

	/**
	 * Signals the object.  This function is only valid in the
	 * player thread.  The object should be locked prior to
	 * calling this function.
	 */
	void Signal() {
		cond.signal();
	}

	/**
	 * Waits for a signal on the #decoder_control object.  This function
	 * is only valid in the decoder thread.  The object must be locked
	 * prior to calling this function.
	 */
	void Wait() {
		cond.wait(mutex);
	}

	/**
	 * Waits for a signal from the decoder thread.  This object
	 * must be locked prior to calling this function.  This method
	 * is only valid in the player thread.
	 */
	void WaitForDecoder() {
		client_cond.wait(mutex);
	}

	bool IsIdle() const {
		return state == DecoderState::STOP ||
			state == DecoderState::ERROR;
	}

	gcc_pure
	bool LockIsIdle() const {
		Lock();
		bool result = IsIdle();
		Unlock();
		return result;
	}

	bool IsStarting() const {
		return state == DecoderState::START;
	}

	gcc_pure
	bool LockIsStarting() const {
		Lock();
		bool result = IsStarting();
		Unlock();
		return result;
	}

	bool HasFailed() const {
		assert(command == DecoderCommand::NONE);

		return state == DecoderState::ERROR;
	}

	gcc_pure
	bool LockHasFailed() const {
		Lock();
		bool result = HasFailed();
		Unlock();
		return result;
	}

	/**
	 * Checks whether an error has occurred, and if so, returns a
	 * copy of the #Error object.
	 *
	 * Caller must lock the object.
	 */
	gcc_pure
	Error GetError() const {
		assert(command == DecoderCommand::NONE);
		assert(state != DecoderState::ERROR || error.IsDefined());

		Error result;
		if (state == DecoderState::ERROR)
			result.Set(error);
		return result;
	}

	/**
	 * Like dc_get_error(), but locks and unlocks the object.
	 */
	gcc_pure
	Error LockGetError() const {
		Lock();
		Error result = GetError();
		Unlock();
		return result;
	}

	/**
	 * Clear the error condition and free the #Error object (if any).
	 *
	 * Caller must lock the object.
	 */
	void ClearError() {
		if (state == DecoderState::ERROR) {
			error.Clear();
			state = DecoderState::STOP;
		}
	}

	/**
	 * Check if the specified song is currently being decoded.  If the
	 * decoder is not running currently (or being started), then this
	 * function returns false in any case.
	 *
	 * Caller must lock the object.
	 */
	gcc_pure
	bool IsCurrentSong(const Song *_song) const;

	gcc_pure
	bool LockIsCurrentSong(const Song *_song) const {
		Lock();
		const bool result = IsCurrentSong(_song);
		Unlock();
		return result;
	}

private:
	/**
	 * Wait for the command to be finished by the decoder thread.
	 *
	 * To be called from the client thread.  Caller must lock the
	 * object.
	 */
	void WaitCommandLocked() {
		while (command != DecoderCommand::NONE)
			WaitForDecoder();
	}

	/**
	 * Send a command to the decoder thread and synchronously wait
	 * for it to finish.
	 *
	 * To be called from the client thread.  Caller must lock the
	 * object.
	 */
	void SynchronousCommandLocked(DecoderCommand cmd) {
		command = cmd;
		Signal();
		WaitCommandLocked();
	}

	/**
	 * Send a command to the decoder thread and synchronously wait
	 * for it to finish.
	 *
	 * To be called from the client thread.  This method locks the
	 * object.
	 */
	void LockSynchronousCommand(DecoderCommand cmd) {
		Lock();
		ClearError();
		SynchronousCommandLocked(cmd);
		Unlock();
	}

	void LockAsynchronousCommand(DecoderCommand cmd) {
		Lock();
		command = cmd;
		Signal();
		Unlock();
	}

public:
	/**
	 * Start the decoder.
	 *
	 * @param song the song to be decoded; the given instance will be
	 * owned and freed by the decoder
	 * @param start_ms see #decoder_control
	 * @param end_ms see #decoder_control
	 * @param pipe the pipe which receives the decoded chunks (owned by
	 * the caller)
	 */
	void Start(Song *song, unsigned start_ms, unsigned end_ms,
		   MusicBuffer &buffer, MusicPipe &pipe);

	void Stop();

	bool Seek(double where);

	void Quit();

	void MixRampStart(char *_mixramp_start);
	void MixRampEnd(char *_mixramp_end);
	void MixRampPrevEnd(char *_mixramp_prev_end);
};

#endif
