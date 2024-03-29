// Copyright (c) 2014-2016, The Regents of the University of California.
// Copyright (c) 2016-2017, Nefeli Networks, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// * Neither the names of the copyright holders nor the names of their
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef BESS_MODULES_QUEUE_H_
#define BESS_MODULES_QUEUE_H_

#include "../kmod/llring.h"
#include "../module.h"
#include "../pb/module_msg.pb.h"
#include <stdio.h>

#include "../utils/format.h"
#include "../utils/endian.h"
#include "../utils/ip.h"
#include <unordered_map>
#include <utility>

using bess::utils::be16_t;
using bess::utils::be32_t;

class Queue : public Module {
public:
    static const Commands cmds;

    Queue()
            : Module(),
              queue_(),
              prefetch_(),
              backpressure_(),
              burst_(),
              size_(),
              high_water_(),
              low_water_(),
              stats_() {
        is_task_ = true;
        propagate_workers_ = false;
        max_allowed_workers_ = Worker::kMaxWorkers;
    }

    CommandResponse Init(const bess::pb::QueueArg &arg);

    CommandResponse GetInitialArg(const bess::pb::EmptyArg &);

    CommandResponse GetRuntimeConfig(const bess::pb::EmptyArg &arg);

    CommandResponse SetRuntimeConfig(const bess::pb::QueueArg &arg);

    void DeInit() override;

    struct task_result RunTask(Context *ctx, bess::PacketBatch *batch,
                               void *arg) override;

    void ProcessBatch(Context *ctx, bess::PacketBatch *batch) override;

    std::string GetDesc() const override;

    CommandResponse CommandSetBurst(const bess::pb::QueueCommandSetBurstArg &arg);

    CommandResponse CommandSetSize(const bess::pb::QueueCommandSetSizeArg &arg);

    CommandResponse CommandGetStatus(
            const bess::pb::QueueCommandGetStatusArg &arg);

    CheckConstraintResult CheckModuleConstraints() const override;

private:
    const double kHighWaterRatio = 0.90;
    const double kLowWaterRatio = 0.15;

    int Resize(int slots);

    // Readjusts the water level according to `size_`.
    void AdjustWaterLevels();

    CommandResponse SetSize(uint64_t size);

    struct llring *queue_;
    bool prefetch_;

    // Whether backpressure should be applied or not
    bool backpressure_;

    int burst_;

    // Queue capacity
    uint64_t size_;

    // High water occupancy
    uint64_t high_water_;

    // Low water occupancy
    uint64_t low_water_;

    // Accumulated statistics counters
    struct {
        uint64_t enqueued;
        uint64_t dequeued;
        uint64_t dropped;
    } stats_;

    bess::pb::QueueArg init_arg_;

    // ADDED BY PHILIP - not part of BESS code
    FILE *drop_log_file = NULL;
    uint64_t init_time_micro;
    void LogDroppedPacket(uint64_t time_ns, bess::utils::be32_t src_ip, bess::utils::be16_t src_port,
                            be16_t dst_port, be32_t seq_num, uint16_t pkt_size);

    FILE *avg_q_size_file = NULL;
    double avg_queue_size;
    uint64_t tot_q_measures;
    uint64_t time_q_logged;
    void LogQueueSize(uint64_t now_ns);
    // logging in-queue packet counts to estimate packets in flight
    FILE *in_q_log_file = NULL;
    uint64_t time_inq_logged;

    struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);

        return h1 ^ h2;
    }
    };

    std::unordered_map<std::pair<be32_t, be16_t>, uint32_t, pair_hash> packets_in_q;
    void LogInQ(uint64_t now_ns);
    void RecordInQStats(bool enq, be32_t src_ip, be16_t src_port);
};

#endif  // BESS_MODULES_QUEUE_H_
