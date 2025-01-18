#include "lsq.h"
#include <stdio.h>

LSQ::LSQ() {
    head = 0;
    tail = 0;
    count = 0;
}

int LSQ::add_entry(bool is_store, uint32_t rob_idx) {
    if (is_full()) {
        printf("LSQ: Cannot add entry - queue full\n");
        return -1;
    }

    LSQ_Entry& entry = entries[tail];
    entry = LSQ_Entry();  // Reset entry
    entry.is_store = is_store;
    entry.rob_index = rob_idx;
    entry.age = count;  // Age for ordering

    int allocated_index = tail;
    tail = (tail + 1) % 6;
    count++;

    printf("LSQ: Added %s at index %d (ROB: %d)\n", 
           is_store ? "STORE" : "LOAD", allocated_index, rob_idx);
    return allocated_index;
}

bool LSQ::can_execute(int index) {
    if (index < 0 || index >= 6) {
        return false;
    }

    LSQ_Entry& entry = entries[index];
    
    // Check if this is the head entry
    if (index != head) {
        printf("LSQ: Entry %d cannot execute - not at head\n", index);
        return false;
    }

    // Check if address is ready
    if (!entry.address_ready) {
        printf("LSQ: Entry %d cannot execute - address not ready\n", index);
        return false;
    }

    // For stores, need data value ready
    if (entry.is_store && !entry.data_ready) {
        printf("LSQ: Entry %d cannot execute - store data not ready\n", index);
        return false;
    }

    return true;  // All conditions met
}

void LSQ::set_address(int index, uint32_t addr) {
    if (index >= 0 && index < 6) {
        entries[index].address = addr;
        entries[index].address_ready = true;
        printf("LSQ: Set address 0x%x for entry %d\n", addr, index);
    }
}

void LSQ::set_data(int index, uint32_t data) {
    if (index >= 0 && index < 6) {
        entries[index].data = data;
        entries[index].data_ready = true;
        printf("LSQ: Set data 0x%x for entry %d\n", data, index);
    }
}

void LSQ::complete_entry(int index) {
    if (index >= 0 && index < 6) {
        entries[index].completed = true;
        printf("LSQ: Completed entry %d\n", index);
    }
}

void LSQ::remove_entry() {
    if (!is_empty()) {
        if (entries[head].completed) {
            printf("LSQ: Removing entry %d (ROB: %d)\n", 
                   head, entries[head].rob_index);
            entries[head] = LSQ_Entry();  // Clear entry
            head = (head + 1) % 6;
            count--;
        } else {
            printf("LSQ: Cannot remove uncompleted entry at head\n");
        }
    }
}

// Dependency Management
void LSQ::set_base_tag(int index, uint32_t tag) {
    if (index >= 0 && index < 6) {
        entries[index].base_reg_tag = tag;
        printf("LSQ: Set base tag %d for entry %d\n", tag, index);
    }
}

void LSQ::set_offset_tag(int index, uint32_t tag) {
    if (index >= 0 && index < 6) {
        entries[index].offset_reg_tag = tag;
        printf("LSQ: Set offset tag %d for entry %d\n", tag, index);
    }
}

void LSQ::set_data_tag(int index, uint32_t tag) {
    if (index >= 0 && index < 6) {
        entries[index].data_reg_tag = tag;
        printf("LSQ: Set data tag %d for entry %d\n", tag, index);
    }
}

void LSQ::update_tag(uint32_t tag, uint32_t value) {
    for (int i = 0; i < 6; i++) {
        LSQ_Entry& entry = entries[i];
        
        // Update base register
        if (!entry.base_ready && entry.base_reg_tag == tag) {
            entry.base_value = value;
            entry.base_ready = true;
            printf("LSQ: Updated base value for entry %d\n", i);
        }
        
        // Update offset register
        if (!entry.offset_ready && entry.offset_reg_tag == tag) {
            entry.offset_value = value;
            entry.offset_ready = true;
            printf("LSQ: Updated offset value for entry %d\n", i);
        }
        
        // Update data register (for stores)
        if (entry.is_store && !entry.data_ready && entry.data_reg_tag == tag) {
            entry.data = value;
            entry.data_ready = true;
            printf("LSQ: Updated data value for entry %d\n", i);
        }
        
        // Try to calculate address if both base and offset are ready
        if (!entry.address_ready && entry.base_ready && entry.offset_ready) {
            entry.address = entry.base_value + entry.offset_value;
            entry.address_ready = true;
            printf("LSQ: Calculated address 0x%x for entry %d\n", 
                   entry.address, i);
        }
    }
}

void LSQ::display_status() {
    printf("\nLSQ Status:\n");
    printf("Head: %d, Tail: %d, Count: %d\n", head, tail, count);
    
    if (!is_empty()) {
        printf("\nValid Entries:\n");
        int current = head;
        int entries_shown = 0;
        
        while (entries_shown < count) {
            LSQ_Entry& entry = entries[current];
            printf("Index %d: %s ROB=%d Addr=0x%x %s\n",
                   current,
                   entry.is_store ? "STORE" : "LOAD ",
                   entry.rob_index,
                   entry.address_ready ? entry.address : 0,
                   entry.completed ? "COMPLETED" : "PENDING");
                   
            if (entry.is_store) {
                printf("         Data=0x%x DataReady=%d\n",
                       entry.data_ready ? entry.data : 0,
                       entry.data_ready);
            }
            
            current = (current + 1) % 6;
            entries_shown++;
        }
    }
}