#include "rob.h"
#include "apex_cpu.h"
#include <stdio.h>

ROB::ROB() {
    head = 0;
    tail = 0;
    count = 0;
}

bool ROB::is_full() {
    return count == 80;
}

bool ROB::is_empty() {
    bool empty = (count == 0);
    printf("is_empty check - Count: %d, Result: %d\n", count, empty);
    return empty;
}

ROB_Entry* ROB::get_entry(int index) {
    if (index >= 0 && index < 80) {
        return &entries[index];
    }
    return nullptr;
}

int ROB::add_entry(uint32_t pc, InstructionType type, 
                   uint32_t dest_arch_reg, uint32_t dest_phys_reg,
                   uint32_t old_phys_reg, uint32_t control_tag) {
    if (is_full()) {
        return -1;  // ROB is full
    }

    // Create new entry
    entries[tail].pc = pc;
    entries[tail].type = type;
    entries[tail].dest_arch_reg = dest_arch_reg;
    entries[tail].dest_phys_reg = dest_phys_reg;
    entries[tail].old_phys_reg = old_phys_reg;
    entries[tail].control_tag = control_tag;
    entries[tail].completed = false;
    entries[tail].mispredicted = false;

    int allocated_index = tail;
    
    // Update tail pointer
    tail = (tail + 1) % 80;
    count++;

    return allocated_index;
}

void ROB::write_result(int rob_idx, uint32_t value, bool mispredict) {
    if (rob_idx >= 0 && rob_idx < 80) {
        entries[rob_idx].value = value;
        entries[rob_idx].completed = true;
        entries[rob_idx].mispredicted = mispredict;
    }
}

bool ROB::commit_entry() {
    printf("Commit Entry - Head: %d, Tail: %d, Count: %d\n", head, tail, count);
    
    if (is_empty()) {
        printf("Cannot commit: ROB is empty\n");
        return false;
    }
    
    if (!entries[head].completed) {
        printf("Cannot commit: Entry at head (index %d) is not completed\n", head);
        return false;
    }

    // Entry is ready to commit
    printf("Committing entry at index %d\n", head);
    
    // Clear the entry
    entries[head] = ROB_Entry();  // Reset entry to default state

    // Update head pointer
    head = (head + 1) % 80;
    count--;

    printf("After commit - Head: %d, Tail: %d, Count: %d\n", head, tail, count);
    return true;
}

void ROB::rollback(int rob_idx) {
    if (rob_idx < 0 || rob_idx >= 80) {
        return; // Invalid index
    }

    // First, verify this is a valid entry within our current ROB bounds
    bool valid_idx = false;
    if (head <= tail) {
        valid_idx = (rob_idx >= head && rob_idx < tail);
    } else {
        valid_idx = (rob_idx >= head || rob_idx < tail);
    }

    if (!valid_idx) {
        return; // Index not in current ROB window
    }

    printf("\nRollback Details (Before):\n");
    printf("Head: %d, Tail: %d, Count: %d, Rollback Index: %d\n", 
           head, tail, count, rob_idx);

    // Calculate new tail position (one after rob_idx)
    int new_tail = (rob_idx + 1) % 80;
    
    // Clear all entries from new_tail to old tail
    int idx = new_tail;
    while (idx != tail) {
        // Clear the entry
        entries[idx] = ROB_Entry(); // Reset to default state
        idx = (idx + 1) % 80;
    }

    // Update tail and recalculate count
    tail = new_tail;
    
    // Recalculate count based on new tail position
    if (head <= tail) {
        count = tail - head;
    } else {
        count = (80 - head) + tail;
    }

    printf("Rollback Details (After):\n");
    printf("Head: %d, Tail: %d, Count: %d\n", head, tail, count);
}

void ROB::display_status() {
    printf("ROB Status:\n");
    printf("Head: %d, Tail: %d, Count: %d\n", head, tail, count);
    
    if (!is_empty()) {
        printf("Valid Entries:\n");
        int i = head;
        int entries_shown = 0;
        while (entries_shown < count) {
            printf("Index %d: PC=0x%x, Type=%d, Completed=%d\n",
                   i, entries[i].pc, entries[i].type, entries[i].completed);
            i = (i + 1) % 80;
            entries_shown++;
        }
    }
}