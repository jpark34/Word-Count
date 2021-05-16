// Authors: Stephen Galucy, Jarrett Parker, Benjamin Medoff
// PSU Emails: sag83@psu.edu, jwp5716@psu.edu, brm5414@psu.edu

#include "buffer.h"

// Creates a buffer with the given capacity
state_t* buffer_create(int capacity)
{
    state_t* buffer = (state_t*) malloc(sizeof(state_t));
    buffer->fifoQ = (fifo_t *) malloc ( sizeof (fifo_t));
    fifo_init(buffer->fifoQ,capacity);
    buffer->isopen = true;

    // Can do any kind of initialization that we may need here

    // We didn't realize that we could use the mutex and CV already given to us in the state_t struct
    // So we made our own and our program worked so we did not change them once we realized it

    // initialize the mutex
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        //return BUFFER_ERROR;
    }

    // initialize the condition variable for the buffer_send function
    // if it does not initialize correctly then destroy all previous initializations
    if (pthread_cond_init(&buffer->sendCond, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        //return BUFFER_ERROR;
    }

    // initialize the condition variable for the buffer_receive function
    // if it does not initialize correctly then destroy all previous initializations
    if (pthread_cond_init(&buffer->receiveCond, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        pthread_cond_destroy(&buffer->sendCond);
        //return BUFFER_ERROR;
    }

    return buffer;
}


// Writes data to the given buffer
// This is a blocking call i.e., the function only returns on a successful completion of send
// In case the buffer is full, the function waits till the buffer has space to write the new data
// Returns BUFFER_SUCCESS for successfully writing data to the buffer,
// CLOSED_ERROR if the buffer is closed, and
// BUFFER_ERROR on encountering any other generic error of any sort
enum buffer_status buffer_send(state_t *buffer, void* data)
{
    pthread_mutex_lock(&buffer->mutex);

    // check to see if the buffer is closed
    if(!buffer->isopen)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return CLOSED_ERROR;
    }

    int msg_size = get_msg_size(data);

    // While there isn't enough room in the buffer for the message,
    // Wait on the condition variable until there is enough room
    // Multiple checks to make sure that the buffer stays open after waking up
    while (fifo_avail_size(buffer->fifoQ) <= msg_size) {
        pthread_cond_wait(&buffer->sendCond, &buffer->mutex);

        // check to see if the buffer is closed
        if(!buffer->isopen)
        {
            pthread_mutex_unlock(&buffer->mutex);
            return CLOSED_ERROR;
        }
    }

    // check to see if the buffer is closed
    if(!buffer->isopen)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return CLOSED_ERROR;
    }

    // Add the data to the buffer, if there is an error doing so unlock the mutex and return buffer_error
    if ( buffer_add_Q(buffer, data) == BUFFER_ERROR ){
        pthread_mutex_unlock(&buffer->mutex);
        return BUFFER_ERROR;
    }

    // Wake up a thread that is waiting on the condition variable in receive
    // Then unlock the mutex
    pthread_cond_signal(&buffer->receiveCond);
    pthread_mutex_unlock(&buffer->mutex);
    
    return BUFFER_SUCCESS;
    
    /*if(fifo_avail_size(buffer->fifoQ) > msg_size )
    {
        // check to see if the buffer is closed
        if(!buffer->isopen)
        {
            pthread_mutex_unlock(&buffer->mutex);
            return CLOSED_ERROR;
        }

	    buffer_add_Q(buffer,data);
        pthread_mutex_unlock(&buffer->mutex);
    	return BUFFER_SUCCESS;	
    }
    else
        // check to see if the buffer is closed
        if(!buffer->isopen)
        {
            pthread_mutex_unlock(&buffer->mutex);
            return CLOSED_ERROR;
        }

        pthread_mutex_unlock(&buffer->mutex);
        return BUFFER_ERROR;
    */
}
// test_send_correctness 1
// Reads data from the given buffer and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This is a blocking call i.e., the function only returns on a successful completion of receive
// In case the buffer is empty, the function waits till the buffer has some data to read
// Return BUFFER_SPECIAL_MESSSAGE for successful retrieval of special data "splmsg"
// Returns BUFFER_SUCCESS for successful retrieval of any data other than "splmsg"
// CLOSED_ERROR if the buffer is closed, and
// BUFFER_ERROR on encountering any other generic error of any sort
enum buffer_status buffer_receive(state_t* buffer, void** data)
{
    pthread_mutex_lock(&buffer->mutex);

    // check to see if the buffer is closed
    if(!buffer->isopen)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return CLOSED_ERROR;
    }

    // While there is no data stored inside of the buffer to be removed
    // Wait on the condition variable until there is something added to the buffer to be removed
    // Multiple checks to make sure that the buffer stays open after waking up
    while (fifo_used_size(buffer->fifoQ) == 0){
        pthread_cond_wait(&buffer->receiveCond, &buffer->mutex);

        // check to see if the buffer is closed
        if(!buffer->isopen)
        {
            pthread_mutex_unlock(&buffer->mutex);
            return CLOSED_ERROR;
        }
    }

    // check to see if the buffer is closed
    if(!buffer->isopen)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return CLOSED_ERROR;
    }

    // Remove data from the buffer and store it in the variable data, else unlock the mutex and return buffer_error
    if ( buffer_remove_Q(buffer, data) == BUFFER_ERROR ){
        pthread_mutex_unlock(&buffer->mutex);
        return BUFFER_ERROR;
    }

    // See if the data removed from the buffer matches a specific message
    // If the removed message matches a certain message then signal a waiting thread, unlock the mutex, and return a special message
    if ( strcmp(*(char**)(data), "splmsg") == 0 ) {
        pthread_cond_signal(&buffer->sendCond);
        pthread_mutex_unlock(&buffer->mutex);

        return BUFFER_SPECIAL_MESSSAGE;
    }
    
    // Wake up a thread waiting in send
    // Then unlock the mutex
    pthread_cond_signal(&buffer->sendCond);
    pthread_mutex_unlock(&buffer->mutex);
    
    return BUFFER_SUCCESS;
    
    /*if(buffer->fifoQ->avilSize < buffer->fifoQ->size)  // checking if there is something in the Q to remove
    {
    	buffer_remove_Q(buffer,data);
    	if(strcmp(*(char**)(data),"splmsg") ==0)
    	{
        	return BUFFER_SPECIAL_MESSSAGE;
    	}
    	return BUFFER_SUCCESS;
    }
    else
        return BUFFER_ERROR;
    */
}


// Closes the buffer and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the buffer is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns BUFFER_SUCCESS if close is successful,
// CLOSED_ERROR if the buffer is already closed, and
// BUFFER_ERROR in any other error case
enum buffer_status buffer_close(state_t* buffer)
{
    pthread_mutex_lock(&buffer->mutex);

    // check to see if the buffer is closed
    if(!buffer->isopen)
    {
        pthread_mutex_unlock(&buffer->mutex);
        return CLOSED_ERROR;
    }

    buffer->isopen = false;

    // broadcast to wake up the next thread to run now that this one is done
    pthread_cond_broadcast(&buffer->sendCond);
    pthread_cond_broadcast(&buffer->receiveCond);

    pthread_mutex_unlock(&buffer->mutex);

    return BUFFER_SUCCESS;
    
}

// Frees all the memory allocated to the buffer , using own version of sem flags
// The caller is responsible for calling buffer_close and waiting for all threads to finish their tasks before calling buffer_destroy
// Returns BUFFER_SUCCESS if destroy is successful,
// DESTROY_ERROR if buffer_destroy is called on an open buffer, and
// BUFFER_ERROR in any other error case
enum buffer_status buffer_destroy(state_t* buffer)
{
    // check to see if the buffer is open
    if(buffer->isopen)
    {
        return DESTROY_ERROR;
    }

    // destroy the mutex and condition variables
    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->sendCond);
    pthread_cond_destroy(&buffer->receiveCond);

    // free the queue and the buffer itself
    fifo_free(buffer->fifoQ);
    free(buffer);
    return BUFFER_SUCCESS;
}