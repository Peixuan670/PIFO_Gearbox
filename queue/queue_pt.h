/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/nsnam/ns-2/queue/queue.h,v 1.35 2005/01/13 18:33:48 haldar Exp $ (LBL)
 */

#ifndef ns_queue_h
#define ns_queue_h

#include "connector.h"
#include "packet.h"
#include "Packet_metadata.h"
#include "ip.h"
class Packet;
class Packet_metadata;

class PacketQueue_pt : public TclObject {
public:
	PacketQueue_pt() : head_(0), tail_(0), len_(0), bytes_(0) {}
	virtual int length() const { return (len_); }
	virtual int byteLength() const { return (bytes_); }
	virtual Packet_metadata* enque(Packet_metadata* p) { // Returns previous tail
		Packet_metadata* pt = tail_;
		if (!tail_) head_= tail_= p;
		else {
			tail_->next_= p;
			tail_= p;
		}
		tail_->next_= 0;
		++len_;
		bytes_ += hdr_cmn::access(p->access_pkt())->size();
		return pt;
	}
	virtual Packet_metadata* deque() {
		if (!head_) return 0;
		Packet_metadata* p = head_;
		head_= p->next_; // 0 if p == tail_
		if (p == tail_) head_= tail_= 0;
		--len_;
		bytes_ -= hdr_cmn::access(p->access_pkt())->size();
		return p;
	}
	Packet_metadata* lookup(int n) {
		for (Packet_metadata* p = head_; p != 0; p = p->next_) {
			if (--n < 0)
				return (p);
		}
		return (0);
	}
	/* remove a specific packet, which must be in the queue */
	virtual void remove(Packet_metadata*);
	/* Remove a packet, located after a given packet. Either could be 0. */
	void remove(Packet_metadata *, Packet_metadata *);
        Packet_metadata* head() { return head_; }
	Packet_metadata* tail() { return tail_; }
	// MONARCH EXTNS
	virtual inline void enqueHead(Packet_metadata* p) {
	        if (!head_) tail_ = p;
	        p->next_ = head_;
		head_ = p;
		++len_;
		bytes_ += hdr_cmn::access(p->access_pkt())->size();
	}
        void resetIterator() {iter = head_;}
        Packet_metadata* getNext() { 
	        if (!iter) return 0;
		Packet_metadata *tmp = iter; iter = iter->next_;
		return tmp;
	}

protected:
	Packet_metadata* head_;
	Packet_metadata* tail_;
	int len_;		// packet count
	int bytes_;		// queue size in bytes


// MONARCH EXTNS
private:
	Packet_metadata *iter;
};

class Queue_pt;

class QueueHandler_pt : public Handler {
public:
	inline QueueHandler_pt(Queue_pt& q) : queue_(q) {}
	void handle(Event*);
private:
	Queue_pt& queue_;
};


class Queue_pt : public Connector {
public:
	virtual void enque(Packet_metadata*) = 0;
	virtual Packet_metadata* deque() = 0;
	virtual void recv(Packet_metadata*, Handler*);
	virtual void updateStats(int queuesize); 
	void resume();
	
	int blocked() const { return (blocked_ == 1); }
	void unblock() { blocked_ = 0; }
	void block() { blocked_ = 1; }
	int limit() { return qlim_; }
	int length() { return pq_->length(); }	/* number of pkts currently in
						 * underlying packet queue */
	int byteLength() { return pq_->byteLength(); }	/* number of bytes *
						 * currently in packet queue */
	/* mean utilization, decaying based on util_weight */
	virtual double utilization (void);

	/* max utilization over recent time period.
	   Returns the maximum of recent measurements stored in util_buf_*/
	double peak_utilization(void);
	virtual ~Queue_pt();
protected:
	Queue_pt();
	void reset();
	int qlim_;		/* maximum allowed pkts in queue */
	int blocked_;		/* blocked now? */
	int unblock_on_resume_;	/* unblock q on idle? */
	QueueHandler_pt qh_;
	PacketQueue_pt *pq_;	/* pointer to actual packet queue 
				 * (maintained by the individual disciplines
				 * like DropTail and RED). */
	double true_ave_;	/* true long-term average queue size */
	double total_time_;	/* total time average queue size compute for */


	void utilUpdate(double int_begin, double int_end, int link_state);
	double last_change_;  /* time at which state changed/utilization measured */
	double old_util_;     /* current utilization */ 
	double util_weight_;  /* decay factor for measuring the link utilization */
	double util_check_intv_; /* interval for reseting the current
				    utilization measurements (seconds) */
	double period_begin_;	/* time of starting the current utilization
				   measurement */
	double cur_util_;	/* utilization during current time period */
	int buf_slot_;		/* Currently active utilization buffer */
	double *util_buf_;    /* Buffer for recent utilization measurements */
	int util_records_;	/* Number of recent utilization measurements
				   stored in memory. One slot in buffer holds
				   period of util_check_intv_ seconds. */
	// measuring #drops
	
};

#endif
