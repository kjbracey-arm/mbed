/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MBED_FILEHANDLE_H
#define MBED_FILEHANDLE_H

typedef int FILEHANDLE;

#include <cstdio>
#include "Callback.h"
#include "platform/mbed_poll.h"
#include "platform/platform.h"
#include "platform/NonCopyable.h"
#include "platform/ConditionVariableCS.h"

namespace mbed {
/** \addtogroup platform */
/** @{*/
/**
 * \defgroup platform_FileHandle FileHandle functions
 * @{
 */


/** Class FileHandle
 *
 *  An abstract interface that represents operations on a file-like
 *  object. The core functions are read, write, and seek, but only
 *  a subset of these operations can be provided.
 *
 *  @note to create a file, @see File
 *  @note Synchronization level: Set by subclass
 */
class FileHandle : private NonCopyable<FileHandle> {
public:
    virtual ~FileHandle() {}

    /** Read the contents of a file into a buffer
     *
     *  Devices acting as FileHandles should follow POSIX semantics:
     *
     *  * if no data is available, and non-blocking set return -EAGAIN
     *  * if no data is available, and blocking set, wait until some data is available
     *  * If any data is available, call returns immediately
     *
     *  @param buffer   The buffer to read in to
     *  @param size     The number of bytes to read
     *  @return         The number of bytes read, 0 at end of file, negative error on failure
     */
    virtual ssize_t read(void *buffer, size_t size) = 0;

    /** Write the contents of a buffer to a file
     *
     *  Devices acting as FileHandles should follow POSIX semantics:
     *
     * * if blocking, block until all data is written
     * * if no data can be written, and non-blocking set, return -EAGAIN
     * * if some data can be written, and non-blocking set, write partial
     *
     *  @param buffer   The buffer to write from
     *  @param size     The number of bytes to write 
     *  @return         The number of bytes written, negative error on failure
     */
    virtual ssize_t write(const void *buffer, size_t size) = 0;

    /** Move the file position to a given offset from from a given location
     *
     *  @param offset   The offset from whence to move to
     *  @param whence   The start of where to seek
     *      SEEK_SET to start from beginning of file,
     *      SEEK_CUR to start from current position in file,
     *      SEEK_END to start from end of file
     *  @return         The new offset of the file, negative error code on failure
     */
    virtual off_t seek(off_t offset, int whence = SEEK_SET) = 0;

    /** Close a file
     *
     *  @return         0 on success, negative error code on failure
     */
    virtual int close() = 0;

    /** Flush any buffers associated with the file
     *
     *  @return         0 on success, negative error code on failure
     */
    virtual int sync()
    {
        return 0;
    }

    /** Check if the file in an interactive terminal device
     *
     *  @return         True if the file is a terminal
     *  @return         False if the file is not a terminal
     *  @return         Negative error code on failure
     */
    virtual int isatty()
    {
        return false;
    }

    /** Get the file position of the file
     *
     *  @note This is equivalent to seek(0, SEEK_CUR)
     *
     *  @return         The current offset in the file, negative error code on failure
     */
    virtual off_t tell()
    {
        return seek(0, SEEK_CUR);
    }

    /** Rewind the file position to the beginning of the file
     *
     *  @note This is equivalent to seek(0, SEEK_SET)
     */
    virtual void rewind()
    {
        seek(0, SEEK_SET);
    }

    /** Get the size of the file
     *
     *  @return         Size of the file in bytes
     */
    virtual off_t size();

    /** Move the file position to a given offset from a given location.
     *
     *  @param offset The offset from whence to move to
     *  @param whence SEEK_SET for the start of the file, SEEK_CUR for the
     *   current file position, or SEEK_END for the end of the file.
     *
     *  @returns
     *    new file position on success,
     *    -1 on failure or unsupported
     */
    MBED_DEPRECATED_SINCE("mbed-os-5.4", "Replaced by FileHandle::seek")
    virtual off_t lseek(off_t offset, int whence)
    {
        return seek(offset, whence);
    }

    /** Flush any buffers associated with the FileHandle, ensuring it
     *  is up to date on disk
     *
     *  @returns
     *    0 on success or un-needed,
     *   -1 on error
     */
    MBED_DEPRECATED_SINCE("mbed-os-5.4", "Replaced by FileHandle::sync")
    virtual int fsync()
    {
        return sync();
    }

    /** Find the length of the file
     *
     *  @returns
     *   Length of the file
     */
    MBED_DEPRECATED_SINCE("mbed-os-5.4", "Replaced by FileHandle::size")
    virtual off_t flen()
    {
        return size();
    }

    /** Set blocking or non-blocking mode of the file operation like read/write.
     *  Definition depends upon the subclass implementing FileHandle.
     *  The default is blocking.
     *
     *  @param blocking     true for blocking mode, false for non-blocking mode.
     *
     *  @return             0 on success
     *  @return             Negative error code on failure
     */
    virtual int set_blocking(bool blocking)
    {
        return -1;
    }

    /** Check for poll event flags
     * The input parameter can be used or ignored - the could always return all events,
     * or could check just the events listed in events.
     * Call is non-blocking - returns instantaneous state of events.
     * Whenever an event occurs, the derived class should call the sigio() callback.
     *
     * @param events        bitmask of poll events we're interested in - POLLIN/POLLOUT etc.
     *
     * @returns             bitmask of poll events that have occurred.
     */
    virtual short poll(short events) const
    {
        // Possible default for real files
        return POLLIN | POLLOUT;
    }

    /** Check for poll event flags
     * The input parameter can be used or ignored - the could always return all events,
     * or could check just the events listed in events.
     * Call is non-blocking - returns instantaneous state of events.
     * Always called from thread context in a critical section.
     *
     * If `wake` is true, and the call does not return any of the specified
     * events, then the next time any of the specified events occur,
     * wake_poll() must be called.
     *
     * @param events        bitmask of poll events we're interested in - POLLIN/POLLOUT etc.
     * @param wake          if wake is required for these bits
     *
     * @returns             bitmask of poll events that have occurred.
     * @returns             POLLNVAL if device does not support wake functionality
     */
    virtual short poll_with_wake(short events, bool wake)
    {
        // Backwards compatibility - pre-existing FileHandles won't support wake
        return POLLNVAL;
    }

    /** Definition depends upon the subclass implementing FileHandle.
     *  For example, if the FileHandle is of type Stream, writable() could return
     *  true when there is ample buffer space available for write() calls.
     *
     * @returns             true if the FileHandle is writable.
     */
    bool writable() const
    {
        return poll(POLLOUT) & POLLOUT;
    }

    /** Definition depends upon the subclass implementing FileHandle.
     *  For example, if the FileHandle is of type Stream, readable() could return
     *  true when there is something available to read.
     *
     *  @returns            true when there is something available to read.
     */
    bool readable() const
    {
        return poll(POLLIN) & POLLIN;
    }

    /** Register a callback on state change of the file.
     *
     *  The specified callback will be called on state changes such as when
     *  the file can be written to or read from.
     *
     *  The callback may be called in an interrupt context and should not
     *  perform expensive operations.
     *
     *  Note! This is not intended as an attach-like asynchronous api, but rather
     *  as a building block for constructing  such functionality.
     *
     *  The exact timing of when the registered function
     *  is called is not guaranteed and susceptible to change. It should be used
     *  as a cue to make read/write/poll calls to find the current state.
     *
     *  @param func     Function to call on state change
     */
    virtual void sigio(Callback<void()> func)
    {
        //Default for real files. Do nothing for real files.
    }

protected:
    /** Wake up calls to poll()
     *
     * Called by derived class when events occur. Must be called in response
     * to poll_with_wake() - see poll_with_wake for more details.
     *
     * Spurious calls are permitted.
     *
     * @param events bitmask of poll events that have occurred
     */
    void wake_poll(short events);
};

/** Class FileHandleDeviceWakeHelper
 *
 *  This class acts as a helper to implement wake and blocking functionality on top
 *  of an underlying device implementation that is natively non-blocking.
 */
class FileHandleDeviceWakeHelper : public FileHandle {
public:
    /** Read the contents of a file into a buffer
     *
     *  Devices acting as FileHandles should follow POSIX semantics:
     *
     *  * if no data is available, and non-blocking set return -EAGAIN
     *  * if no data is available, and blocking set, wait until some data is available
     *  * If any data is available, call returns immediately
     *
     *  @param buffer   The buffer to read in to
     *  @param size     The number of bytes to read
     *  @return         The number of bytes read, 0 at end of file, negative error on failure
     */
    virtual ssize_t read(void *buffer, size_t size);

    /** Write the contents of a buffer to a file
     *
     *  Devices acting as FileHandles should follow POSIX semantics:
     *
     * * if blocking, block until all data is written
     * * if no data can be written, and non-blocking set, return -EAGAIN
     * * if some data can be written, and non-blocking set, write partial
     *
     *  @param buffer   The buffer to write from
     *  @param size     The number of bytes to write
     *  @return         The number of bytes written, negative error on failure
     */
    virtual ssize_t write(const void *buffer, size_t size);

    virtual int set_blocking(bool blocking) { _blocking = blocking; return 0; }

    /** Check for poll event flags
     * The input parameter can be used or ignored - the could always return all events,
     * or could check just the events listed in events.
     * Call is non-blocking - returns instantaneous state of events.
     *
     * Classes derived from FileHandleDeviceWakeHelper must implement poll(),
     * and must call wake() whenever any event occurs.
     *
     * This is used to implement both FileHandle::poll and FileHandle::poll_with_wake,
     * so will be called from thread context, and may or may not be in a
     * critical section.
     *
     * @param events        bitmask of poll events we're interested in - POLLIN/POLLOUT etc.
     *
     * @returns             bitmask of poll events that have occurred.
     */
    virtual short poll(short events) const  = 0;

    /** Check for poll event flags
     * The input parameter can be used or ignored - the could always return all events,
     * or could check just the events listed in events.
     * Call is non-blocking - returns instantaneous state of events.
     * Always called from thread context in a critical section.
     *
     * If `wake` is true, and the call does not return any of the specified
     * events, then the next time any of the specified events occur,
     * wake_poll() must be called.
     *
     * @param events        bitmask of poll events we're interested in - POLLIN/POLLOUT etc.
     * @param wake          if wake is required for these bits
     *
     * @returns             bitmask of poll events that have occurred.
     * @returns             POLLNVAL if device does not support wake functionality
     */
    virtual short poll_with_wake(short events, bool wake);

    /** Register a callback on state change of the file.
     *
     *  The specified callback will be called on state changes such as when
     *  the file can be written to or read from.
     *
     *  The callback may be called in an interrupt context and should not
     *  perform expensive operations.
     *
     *  Note! This is not intended as an attach-like asynchronous api, but rather
     *  as a building block for constructing  such functionality.
     *
     *  The exact timing of when the registered function
     *  is called is not guaranteed and susceptible to change. It should be used
     *  as a cue to make read/write/poll calls to find the current state.
     *
     *  @param func     Function to call on state change
     */
    virtual void sigio(Callback<void()> func);

protected:
    FileHandleDeviceWakeHelper() : _blocking(true), _poll_wake_events(0) { }

    bool is_blocking() const { return _blocking; }

    /** Indicate whether datagram or stream semantics are required.
     *
     * Behaviour for write differs for datagrams and streams. For a stream,
     * blocking write may make multiple successful calls to write_nonblocking,
     * summing the return values, until the total amount has been written. For a
     * datagram, blocking write will only one successful call to write_nonblocking,
     * returning its value.
     *
     * Read behaviour is the same in both cases - blocking read will make only
     * one successful call to read_nonblocking, as read() is required to
     * return immediately as soon as any data is available for a device.
     *
     * @returns        true for stream semantics
     *                 false for datagram semantics
     */
    virtual bool is_stream() const = 0;

    /** Read the contents of a file into a buffer
     *
     *  Devices acting as FileHandles should follow POSIX semantics, in their
     *  non-blocking form here. FileHandleBlockingHelper will provide blocking
     *  semantics for FileHandle::read() based on this method.
     *
     *  * If no data is available, return -EAGAIN
     *  * If any data is available, call returns immediately
     *
     *  @param buffer   The buffer to read in to
     *  @param size     The number of bytes to read
     *  @return         The number of bytes read, 0 at end of file, negative error on failure
     */
    virtual int read_nonblocking(void *buffer, size_t size) = 0;

    /** Write the contents of a buffer to a file
     *
     *  Devices acting as FileHandles should follow POSIX semantics, in their
     *  non-blocking form here. FileHandleBlockingHelper will provide blocking
     *  semantics for FileHandle::write() based on this method.
     *
     * * if no data can be written return -EAGAIN
     * * if some data can be written, write as much as possible and return immediately
     *
     *  @param buffer   The buffer to write from
     *  @param size     The number of bytes to write
     *  @return         The number of bytes written, negative error on failure
     */
    virtual ssize_t write_nonblocking(const void *buffer, size_t size) = 0;

    /** Wake up on events
     *
     * This must be called by the derived class when events occur. It must
     * occur on all events, and is used by FileHandleBlockingDeviceHelper to
     * implement blocking reads and writes, poll_with_wake and sigio callbacks.
     *
     * Blocking reads will wake on POLLIN or POLLERR. Blocking writes will
     * wake on POLLOUT, POLLHUP or POLLERR.
     *
     * Spurious calls are permitted.
     *
     * @param events bitmask of poll events that have occurred
     */
    void wake(short events);

private:
    bool _blocking;
    short _poll_wake_events;
    ConditionVariableCS _cv_rx;
    ConditionVariableCS _cv_tx;
    Callback<void()> _sigio_cb;
};

/** Not a member function
 *  This call is equivalent to posix fdopen().
 *  It associates a Stream to an already opened file descriptor (FileHandle)
 *
 *  @param fh       a pointer to an opened file descriptor
 *  @param mode     operation upon the file descriptor, e.g., 'wb+'
 *
 *  @returns        a pointer to std::FILE
*/

std::FILE *fdopen(FileHandle *fh, const char *mode);

/**@}*/

/**@}*/


} // namespace mbed

#endif
