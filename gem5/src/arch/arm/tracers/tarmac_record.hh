/*
 * Copyright (c) 2017-2019 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file: The file contains the informations used to generate records
 *        for the pre-ARMv8 cores.
 */

#ifndef __ARCH_ARM_TRACERS_TARMAC_RECORD_HH__
#define __ARCH_ARM_TRACERS_TARMAC_RECORD_HH__

#include <memory>

#include "arch/arm/regs/misc.hh"
#include "arch/arm/tracers/tarmac_base.hh"
#include "base/printable.hh"
#include "cpu/reg_class.hh"
#include "cpu/static_inst.hh"

namespace gem5
{

namespace Trace {

class TarmacContext;

class TarmacTracer;

/**
 * Returns the string representation of the instruction set being
 * currently run according to the Tarmac format.
 *
 * @param isetstate: enum variable (ISetState) specifying an ARM
 *                   instruction set.
 * @return instruction set in string format.
 */
std::string
iSetStateToStr(TarmacBaseRecord::ISetState isetstate);

/**
 * Returns the string representation of the ARM Operating Mode
 * (CPSR.M[3:0] field) according to the Tarmac format.
 *
 * @param opMode: ARM operating mode.
 * @return operating mode in string format.
 */
std::string
opModeToStr(ArmISA::OperatingMode opMode);

/**
 * TarmacTracer Record:
 * Record generated by the TarmacTracer for every executed instruction.
 * The record is composed by a set of entries, matching the tracing
 * capabilities provided by the Tarmac specifications:
 *
 * - Instruction Entry
 * - Register Entry
 * - Memory Entry
 */
class TarmacTracerRecord : public TarmacBaseRecord
{
  public:
    /** Instruction Entry */
    struct TraceInstEntry: public InstEntry, Printable
    {
        TraceInstEntry(const TarmacContext& tarmCtx, bool predicate);

        virtual void print(std::ostream& outs,
                           int verbosity = 0,
                           const std::string &prefix = "") const override;

      protected:
        /** Number of instructions being traced */
        static uint64_t instCount;

        /** True if instruction is executed in secure mode */
        bool secureMode;
        /**
         * Instruction size:
         * 16 for 16-bit Thumb Instruction
         * 32 otherwise (ARM and BigThumb)
         */
        uint8_t instSize;
    };

    /** Register Entry */
    struct TraceRegEntry: public RegEntry, Printable
    {
      public:
        TraceRegEntry(const TarmacContext& tarmCtx, const RegId& reg);

        /**
         * This updates the register entry using the update table. It is
         * a required step after the register entry generation.
         * If unupdated, the entry will be marked as invalid.
         * The entry update cannot be done automatically at TraceRegEntry
         * construction: the entries are extended by consequent Tarmac
         * Tracer versions (like V8), and virtual functions should
         * be avoided during construction.
         */
        void update(const TarmacContext& tarmCtx);

        virtual void print(std::ostream& outs,
                           int verbosity = 0,
                           const std::string &prefix = "") const override;

      protected:
        /** Register update functions. */
        virtual void
        updateMisc(const TarmacContext& tarmCtx, RegIndex regRelIdx);

        virtual void
        updateCC(const TarmacContext& tarmCtx, RegIndex regRelIdx);

        virtual void
        updateFloat(const TarmacContext& tarmCtx, RegIndex regRelIdx);

        virtual void
        updateInt(const TarmacContext& tarmCtx, RegIndex regRelIdx);

        virtual void
        updateVec(const TarmacContext& tarmCtx, RegIndex regRelIdx) {};

        virtual void
        updatePred(const TarmacContext& tarmCtx, RegIndex regRelIdx) {};

      public:
        /** True if register entry is valid */
        bool regValid;
        /** Register class */
        RegClass regClass;
        /** Register arch number */
        RegIndex regRel;
        /** Register name to be printed */
        std::string regName;
    };

    /** Memory Entry */
    struct TraceMemEntry: public MemEntry, Printable
    {
      public:
        TraceMemEntry(const TarmacContext& tarmCtx,
                      uint8_t _size, Addr _addr, uint64_t _data);

        virtual void print(std::ostream& outs,
                           int verbosity = 0,
                           const std::string &prefix = "") const override;

      protected:
        /** True if memory access is a load */
        bool loadAccess;
    };

  public:
    TarmacTracerRecord(Tick _when, ThreadContext *_thread,
                       const StaticInstPtr _staticInst, ArmISA::PCState _pc,
                       TarmacTracer& _tracer,
                       const StaticInstPtr _macroStaticInst = NULL);

    virtual void dump() override;

    using InstPtr = std::unique_ptr<TraceInstEntry>;
    using MemPtr = std::unique_ptr<TraceMemEntry>;
    using RegPtr = std::unique_ptr<TraceRegEntry>;

  protected:
    /** Generates an Entry for the executed instruction. */
    virtual void addInstEntry(std::vector<InstPtr>& queue,
                              const TarmacContext& ptr);

    /** Generates an Entry for every triggered memory access */
    virtual void addMemEntry(std::vector<MemPtr>& queue,
                             const TarmacContext& ptr);

    /** Generate an Entry for every register being written. */
    virtual void addRegEntry(std::vector<RegPtr>& queue,
                             const TarmacContext& ptr);

  protected:
    /** Generate and update a register entry. */
    template<typename RegEntry>
    RegEntry
    genRegister(const TarmacContext& tarmCtx, const RegId& reg)
    {
        RegEntry single_reg(tarmCtx, reg);
        single_reg.update(tarmCtx);

        return single_reg;
    }

    template<typename RegEntry>
    void
    mergeCCEntry(std::vector<RegPtr>& queue, const TarmacContext& tarmCtx)
    {
        // Find all CC Entries and move them at the end of the queue
        auto it = std::remove_if(
            queue.begin(), queue.end(),
            [] (RegPtr& reg) ->bool { return (reg->regClass == CCRegClass); }
        );

        if (it != queue.end()) {
            // Remove all CC Entries.
            queue.erase(it, queue.end());

            auto is_cpsr = [] (RegPtr& reg) ->bool
            {
                return (reg->regClass == MiscRegClass) &&
                       (reg->regRel == ArmISA::MISCREG_CPSR);
            };

            // Looking for the presence of a CPSR register entry.
            auto cpsr_it = std::find_if(
                queue.begin(), queue.end(), is_cpsr
            );

            // If CPSR entry not present, generate one
            if (cpsr_it == queue.end()) {
                RegId reg(MiscRegClass, ArmISA::MISCREG_CPSR);
                queue.push_back(
                    std::make_unique<RegEntry>(
                        genRegister<RegEntry>(tarmCtx, reg))
                );
            }
        }
    }

    /** Flush queues to the trace output */
    template<typename Queue>
    void flushQueues(Queue& queue);
    template<typename Queue, typename... Args>
    void flushQueues(Queue& queue, Args & ... args);

  protected:
    /** Reference to tracer */
    TarmacTracer& tracer;
};

} // namespace Trace
} // namespace gem5

#endif // __ARCH_ARM_TRACERS_TARMAC_RECORD_HH__
