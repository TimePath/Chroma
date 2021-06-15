#pragma once

#include <kernel/system/descriptors.h>
#include <kernel/system/memory.h>
#include <kernel/system/interrupts.h>
#include <kernel/system/process/process.h>

/************************
 *** Team Kitty, 2021 ***
 ***     Chroma       ***
 ***********************/

#define MAX_CORES 8
#define MAX_PROCESSES 128
#define PROCESS_STACK 65535

static size_t strlen(const char* String) {
    size_t Len = 0;
    while(String[Len] != '\0') {
        Len++;
    }
    return Len;
};

/**
 * @brief All the data a process needs.
 * 
 * Contains all the process-specific structs.
 * Lots of private members, out of necessity
 */
class Process {
    // Tells the scheduler, and system at large, what the current state of the process is
    enum ProcessState {
        PROCESS_AVAILABLE,  // Process is ready to be scheduled
        PROCESS_RUNNING,    // Process is active on the CPU
        PROCESS_WAITING,    // Process is blocked by external activity
        PROCESS_CRASH,      // Process has encountered an error and needs to be culled
        PROCESS_NOT_STARTED,// Process is waiting for bootstrap
        PROCESS_REAP        // Process wants to die
    };

    // The process' buffers, for moving data in and out of the system
    enum BufferTypes {
        STDOUT,
        STDIN,
        STDERR
    };

    // The data that actually represents one of the above buffers.
    struct Buffer {
        uint8_t* Data;
        size_t Length;
        size_t LengthAllocated;
        uint8_t Type; // An entry of BufferTypes.
    } __attribute__((packed));

    // A packet used for IPC
    struct ProcessMessage {
        uint8_t MessageID;
        size_t Source;      // Originating PID
        size_t Destination; // Target PID
        size_t Content;     // Pointer to the data.
        size_t Length;      // Size of the data. End is Content + Length
        size_t Response;
        bool Read;          // True if message has been read
        bool Free;          // True if memory can be reused for something else
    } __attribute__((packed));

    // Important information about the process.
    // Its' stack and stack pointer, plus the page tables.
    struct ProcessHeader {
        uint8_t* Stack;
        size_t RSP;
        address_space_t* AddressSpace;
    };

    private:
        ProcessHeader Header;
        ProcessState State;

        bool User;        // Is this process originated in userspace?

        size_t UniquePID; // Globally Unique ID.
        size_t KernelPID; // If in kernel space, the PID.
        size_t ParentPID; // If this process was forked, the parent's PID.

        char Name[128];
        size_t Entry;     // The entry point
        uint8_t Core;

        bool ORS = false;
        bool IsActive = false;
        bool IsInterrupted = false; // True if an interrupt was fired while this process is active
        
        uint8_t Signals[8]; // Interrupt / IRQ / Signal handlers.
        uint8_t Sleeping;   // 0 if active, else the process is waiting for something. TODO: remove this, use State?

        ProcessMessage* Messages; // A queue of IPC messages.
        size_t LastMessage; // The index of the current message.

        uint8_t* ProcessMemory; 
        size_t ProcessMemorySize;

        // TODO: Stack Trace & MFS

    public:

        Process(size_t KPID) : State(PROCESS_AVAILABLE), UniquePID(-1), KernelPID(KPID) {
        };
        
        Process(const char* ProcessName, size_t KPID, size_t UPID, size_t EntryPoint, bool Userspace)
            : UniquePID(UPID), KernelPID(KPID), Entry(EntryPoint), ORS(false), LastMessage(0), User(Userspace), Sleeping(0) {
            
            memcpy((void*) ProcessName, Name, strlen(Name) + 1);
        };

        Process(const Process &Instance) {
            memcpy(this, &Instance, sizeof(Process));
        };

        Process& operator=(const Process &Instance) {
            memcpy(this, &Instance, sizeof(Process));
            return *this;
        };

        /*************************************************************/

        void InitMemory();
        
        void InitMessages();

        /*************************************************************/
        
        void SetCore(size_t CoreID) { Core = CoreID; };

        void IncreaseSleep(size_t Interval) { Sleeping += Interval; };
        void DecreaseSleep(size_t Interval) { Sleeping -= Interval; };

        /*************************************************************/

        ProcessHeader* GetHeader() { return &Header; };

        size_t GetParent() const { return ParentPID; };

        bool IsValid() const { return KernelPID != 0; };

        bool IsUsed() const { return (State != ProcessState::PROCESS_AVAILABLE && State != ProcessState::PROCESS_CRASH && State != ProcessState::PROCESS_REAP) && IsValid(); };

        bool IsSleeping() const { return Sleeping; };

        size_t GetSleepCounter() const { return Sleeping; };

        bool CanRun(const size_t CPU) const {
            bool flag = !(ORS && !IsActive);

            return State == ProcessState::PROCESS_WAITING && Core == CPU && KernelPID != 0 && flag && !IsSleeping();
        }

        size_t GetCore() const { return Core; };


};

class ProcessManagement {
    public:
        TSS64 TSS[MAX_CORES];
        uint32_t CoreCount = 1;

        ProcessManagement();
        static ProcessManagement* instance();

        void Wait();
        void Initialize();
        void InitialiseCore(int APIC, int ID);
        
        void NotifyAllCores();

        // TODO: Process*
        size_t SwitchContext(INTERRUPT_FRAME* CurrentFrame);
        void MapThreadMemory(size_t from, size_t to, size_t length);
        
        void InitProcess(/*func EntryPoint*/ int argc, char** argv);
        void InitProcessPagetable(bool Userspace);
        void InitProcessArch();
        
        size_t HandleRequest(size_t CPU);


        inline void yield() { __asm __volatile("int 100"); }

};