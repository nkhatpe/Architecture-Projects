#include <stdio.h>
#include "apex_cpu.h"
#include "rob.h"
#include "register_manager.h"
#include "control_predictor.h"
#include "lsq.h"  
#include "memory_fu.h"
#include "int_fu.h"

// Test function to verify ROB operations
void test_rob() {
    printf("\n=== Testing ROB Implementation ===\n");
    ROB rob;

    printf("\nTest 1: Initial State\n");
    printf("Expected: Empty ROB\n");
    printf("Result: IsEmpty=%d, IsFull=%d, Count=%d\n", 
           rob.is_empty(), rob.is_full(), rob.get_count());
    rob.display_status();

    printf("\nTest 2: Adding Entries\n");
    // Add some test entries
    int idx1 = rob.add_entry(0x1000, INT, 1, 10, 5, 0);
    int idx2 = rob.add_entry(0x1004, LOAD, 2, 11, 6, 0);
    int idx3 = rob.add_entry(0x1008, ADD, 3, 12, 7, 0);
    
    printf("Added 3 entries at indices: %d, %d, %d\n", idx1, idx2, idx3);
    printf("After adding entries:\n");
    rob.display_status();

    printf("\nTest 3: Writing Results\n");
    rob.write_result(idx1, 100, false);
    rob.write_result(idx2, 200, false);
    printf("After writing results to first two entries:\n");
    rob.display_status();

    printf("\nTest 4: Committing Entries\n");
    bool commit1 = rob.commit_entry();
    bool commit2 = rob.commit_entry();
    bool commit3 = rob.commit_entry();
    printf("Commit results: %d, %d, %d\n", commit1, commit2, commit3);
    printf("After committing entries:\n");
    rob.display_status();

    printf("\nTest 5: Testing Rollback\n");
    // Instead of trying to clear ROB, we'll work with the current state
    printf("Current ROB state before rollback test:\n");
    rob.display_status();

    // Complete the pending instruction at head
    rob.write_result(2, 300, false);  // Complete the ADD instruction at index 2
    printf("\nAfter completing pending instruction:\n");
    rob.display_status();

    // Now commit it
    rob.commit_entry();
    printf("\nAfter committing pending instruction:\n");
    rob.display_status();

    // Now add new test entries for rollback test
    int rollback_idx1 = rob.add_entry(0x2000, INT, 1, 20, 15, 0);
    int rollback_idx2 = rob.add_entry(0x2004, LOAD, 2, 21, 16, 0);
    int branch_idx = rob.add_entry(0x2008, BRANCH, 3, 22, 17, 1);
    int post_branch_idx1 = rob.add_entry(0x200C, ADD, 4, 23, 18, 1);
    int post_branch_idx2 = rob.add_entry(0x2010, MUL, 5, 24, 19, 1);

    // Complete entries up to branch
    rob.write_result(rollback_idx1, 100, false);
    rob.write_result(rollback_idx2, 200, false);
    rob.write_result(branch_idx, 0, true);  // Branch mispredicted
    rob.write_result(post_branch_idx1, 300, false);
    rob.write_result(post_branch_idx2, 400, false);

    printf("\nBefore rollback (all entries):\n");
    rob.display_status();

    // Rollback to the branch instruction
    printf("\nPerforming rollback to branch at index %d\n", branch_idx);
    rob.rollback(branch_idx);

    printf("\nAfter rollback:\n");
    rob.display_status();

    // Try adding new entries after rollback
    int new_idx = rob.add_entry(0x2014, INT, 6, 25, 20, 2);
    printf("\nAfter adding new entry post-rollback (at index %d):\n", new_idx);
    rob.display_status();

    printf("\nTest 6: Testing Full ROB\n");
    // Try to fill the ROB
    int last_idx = -1;
    for(int i = 0; i < 85; i++) {
        int idx = rob.add_entry(0x3000 + i*4, INT, i, i+30, i+20, 0);
        if(idx != -1) {
            last_idx = idx;
        }
    }
    printf("Last successful entry index: %d\n", last_idx);
    printf("ROB state after filling:\n");
    rob.display_status();
}

void test_register_manager() {
    printf("\n=== Testing Register Manager ===\n");
    RegisterManager reg_mgr;
    
    // Initial state
    printf("\nTest 1: Initial State\n");
    reg_mgr.display_status();
    
    // Allocate registers
    printf("\nTest 2: Register Allocation\n");
    uint32_t p1 = reg_mgr.allocate_physical_register();
    uint32_t p2 = reg_mgr.allocate_physical_register();
    printf("Allocated P%d and P%d\n", p1, p2);
    reg_mgr.display_status();
    
    // Update rename tables
    printf("\nTest 3: Rename Table Updates\n");
    reg_mgr.update_frontend_table(1, p1);  // R1 -> P32
    reg_mgr.update_frontend_table(2, p2);  // R2 -> P33
    reg_mgr.display_status();
    
    // Create checkpoint
    printf("\nTest 4: Checkpoint Creation\n");
    int cp_id = reg_mgr.create_checkpoint(1);
    printf("Created checkpoint %d\n", cp_id);
    
    // More updates
    uint32_t p3 = reg_mgr.allocate_physical_register();
    reg_mgr.update_frontend_table(1, p3);  // R1 -> P34
    printf("After more updates:\n");
    reg_mgr.display_status();
    
    // Restore checkpoint
    printf("\nTest 5: Checkpoint Restoration\n");
    reg_mgr.restore_checkpoint(cp_id);
    reg_mgr.display_status();
} 

void test_control_predictor() {
    printf("\n=== Testing Control Predictor ===\n");
    ControlPredictor predictor;

    printf("\nTest 1: Initial State\n");
    printf("Expected: Empty predictor table and RAS\n");
    predictor.display_status();

    printf("\nTest 2: Branch Prediction (Negative Offset)\n");
    uint32_t target;
    uint32_t branch_pc = 0x1000;
    int32_t offset = -8;
    uint32_t expected_target = branch_pc + offset;

    // First lookup (should miss)
    bool prediction = predictor.lookup_prediction(branch_pc, PRED_BRANCH, offset, target);
    printf("First lookup (miss expected):\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n  Expected Target=0x%x\n", 
           branch_pc, prediction, target, expected_target);
    
    // Establish entry
    printf("\nEstablishing entry for backward branch\n");
    predictor.establish_entry(branch_pc, PRED_BRANCH, offset);
    
    // Second lookup (should hit and predict taken)
    prediction = predictor.lookup_prediction(branch_pc, PRED_BRANCH, offset, target);
    printf("\nSecond lookup (hit, should predict taken):\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n  Expected Target=0x%x\n", 
           branch_pc, prediction, target, expected_target);
    predictor.display_status();

    printf("\nTest 3: Branch Prediction (Positive Offset)\n");
    // Test forward branch
    uint32_t forward_pc = 0x2000;
    offset = 8;
    expected_target = forward_pc + offset;
    
    // First lookup (should miss)
    prediction = predictor.lookup_prediction(forward_pc, PRED_BRANCH, offset, target);
    printf("First lookup (miss expected):\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n  Expected Target=0x%x\n", 
           forward_pc, prediction, target, expected_target);
    
    // Establish entry
    predictor.establish_entry(forward_pc, PRED_BRANCH, offset);
    prediction = predictor.lookup_prediction(forward_pc, PRED_BRANCH, offset, target);
    printf("\nAfter establishment (should use default prediction):\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n", forward_pc, prediction, target);
    
    // Update with actual outcome (taken)
    predictor.update_prediction(forward_pc, true, expected_target);
    prediction = predictor.lookup_prediction(forward_pc, PRED_BRANCH, offset, target);
    printf("\nAfter update (should predict taken):\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n", forward_pc, prediction, target);
    predictor.display_status();

    printf("\nTest 4: JALP and Return Stack\n");
    // Test JALP
    uint32_t jalp_pc = 0x3000;
    offset = 16;  // JALP offset
    expected_target = jalp_pc + offset;
    
    predictor.establish_entry(jalp_pc, PRED_JALP, offset);
    prediction = predictor.lookup_prediction(jalp_pc, PRED_JALP, offset, target);
    printf("JALP prediction:\n");
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n  Expected Target=0x%x\n",
           jalp_pc, prediction, target, expected_target);
    
    // Push return address
    uint32_t return_addr = jalp_pc + 4;
    printf("\nPushing return address: 0x%x\n", return_addr);
    predictor.push_return_address(return_addr);
    
    // Test RET
    uint32_t ret_pc = 0x4000;
    predictor.establish_entry(ret_pc, PRED_RET, 0);
    prediction = predictor.lookup_prediction(ret_pc, PRED_RET, 0, target);
    printf("\nRET prediction (should return to 0x%x):\n", return_addr);
    printf("  PC=0x%x\n  Predicted=%d\n  Target=0x%x\n", ret_pc, prediction, target);
    predictor.display_status();

    printf("\nTest 5: FIFO Replacement\n");
    printf("Filling predictor table (8 entries + 2 more to test replacement)...\n");
    for(int i = 0; i < 10; i++) {
        uint32_t pc = 0x5000 + (i * 4);
        offset = -4;  // All backward branches
        predictor.establish_entry(pc, PRED_BRANCH, offset);
        
        prediction = predictor.lookup_prediction(pc, PRED_BRANCH, offset, target);
        printf("Entry %d: PC=0x%x, Predicted=%d, Target=0x%x\n", 
               i, pc, prediction, target);
    }
    
    printf("\nFinal Predictor State:\n");
    predictor.display_status();

    printf("\nTest 6: Multiple RAS Operations\n");
    // Push multiple return addresses
    printf("Pushing multiple return addresses...\n");
    for(int i = 0; i < 5; i++) {  // Try to push more than RAS size
        uint32_t addr = 0x6000 + (i * 4);
        printf("Pushing: 0x%x\n", addr);
        predictor.push_return_address(addr);
    }
    
    printf("\nPopping return addresses...\n");
    for(int i = 0; i < 6; i++) {  // Try to pop more than we pushed
        uint32_t addr = predictor.pop_return_address();
        printf("Popped: 0x%x\n", addr);
    }
    
    predictor.display_status();
}

void test_lsq() {
    printf("\n=== Testing Load/Store Queue ===\n");
    LSQ lsq;

    printf("\nTest 1: Initial State\n");
    lsq.display_status();

    printf("\nTest 2: Adding Load/Store Entries\n");
    // Add a load entry
    int load_idx = lsq.add_entry(false, 10);  // LOAD with ROB index 10
    printf("Added LOAD at index %d\n", load_idx);
    
    // Add a store entry
    int store_idx = lsq.add_entry(true, 11);  // STORE with ROB index 11
    printf("Added STORE at index %d\n", store_idx);
    
    lsq.display_status();

    printf("\nTest 3: Register Dependency Tracking\n");
    // Set dependencies for load
    lsq.set_base_tag(load_idx, 20);   // Base register is P20
    lsq.set_offset_tag(load_idx, 21);  // Offset register is P21
    
    // Set dependencies for store
    lsq.set_base_tag(store_idx, 22);    // Base register is P22
    lsq.set_offset_tag(store_idx, 23);   // Offset register is P23
    lsq.set_data_tag(store_idx, 24);     // Data register is P24
    
    printf("After setting dependencies:\n");
    lsq.display_status();

    printf("\nTest 4: Updating Dependencies\n");
    // Update values as they become available
    lsq.update_tag(20, 0x1000);  // Base address for load
    lsq.update_tag(21, 0x4);     // Offset for load
    lsq.update_tag(22, 0x2000);  // Base address for store
    lsq.update_tag(23, 0x8);     // Offset for store
    lsq.update_tag(24, 0x42);    // Data for store
    
    printf("After updating dependencies:\n");
    lsq.display_status();

    printf("\nTest 5: Execution Check\n");
    printf("Can execute load (should be true): %d\n", lsq.can_execute(load_idx));
    printf("Can execute store (should be false): %d\n", lsq.can_execute(store_idx));

    printf("\nTest 6: Completing and Removing Entries\n");
    // Complete the load
    lsq.complete_entry(load_idx);
    printf("After completing load:\n");
    lsq.display_status();
    
    // Remove the completed load
    lsq.remove_entry();
    printf("After removing load:\n");
    lsq.display_status();
    
    // Now store should be executable
    printf("Can execute store (should now be true): %d\n", lsq.can_execute(store_idx));

    printf("\nTest 7: Queue Full Condition\n");
    // Try to fill the queue
    printf("Filling queue...\n");
    for(int i = 0; i < 6; i++) {
        int idx = lsq.add_entry((i % 2) == 0, 12 + i);
        if(idx != -1) {
            printf("Added entry at index %d\n", idx);
        } else {
            printf("Failed to add entry (queue full)\n");
        }
    }
    lsq.display_status();

    printf("\nTest 8: Address Calculation\n");
    // First complete and remove the current head entry
    printf("Current LSQ state before cleanup:\n");
    lsq.display_status();

    // Complete current head entry
    lsq.complete_entry(lsq.get_head());  // Complete the entry at head
    lsq.remove_entry();                  // Remove it

    printf("\nAfter completing and removing head entry:\n");
    lsq.display_status();

    // Now add new load with dependencies
    int new_load_idx = lsq.add_entry(false, 20);
    if (new_load_idx != -1) {
        printf("Added new load at index %d\n", new_load_idx);

        // Set and immediately resolve dependencies
        lsq.set_base_tag(new_load_idx, 30);
        lsq.set_offset_tag(new_load_idx, 31);
        lsq.update_tag(30, 0x3000);  // Base
        lsq.update_tag(31, 0x10);    // Offset
        // Should show address as 0x3010

        printf("New load entry after address calculation:\n");
        lsq.display_status();

        // Verify it can execute
        printf("Can execute new load: %d\n", lsq.can_execute(new_load_idx));
    } else {
        printf("Failed to add new load entry\n");
    }

}

void test_memory_fu() {
    printf("\n=== Testing Memory Function Unit ===\n");
    LSQ lsq;  // Create LSQ instance first
    MemoryFU mem_fu(lsq);

    printf("\nTest 1: Initial State\n");
    mem_fu.display_status();

    printf("\nTest 2: Basic Load Operation\n");
    // First, set up LSQ entry for load
    int load_idx = lsq.add_entry(false, 100);  // LOAD with ROB index 100
    printf("Created LOAD in LSQ at index %d\n", load_idx);
    
    // Set up and resolve dependencies
    lsq.set_base_tag(load_idx, 20);
    lsq.set_offset_tag(load_idx, 21);
    lsq.update_tag(20, 0x100);  // Base address
    lsq.update_tag(21, 0x4);    // Offset
    // Should result in address 0x104
    
    printf("\nLSQ state after setting up LOAD:\n");
    lsq.display_status();
    
    // Try to issue to MemFU
    if(mem_fu.can_accept()) {
        printf("Issuing LOAD to MemFU\n");
        mem_fu.issue(load_idx);
    }
    
    printf("\nMemFU state after issuing LOAD:\n");
    mem_fu.display_status();
    
    printf("\nTest 3: Pipeline Operation\n");
    // Execute several cycles to move through pipeline
    for(int i = 0; i < 4; i++) {
        printf("\nExecuting cycle %d:\n", i+1);
        mem_fu.execute();
        mem_fu.display_status();
    }

    printf("\nTest 4: Store Operation\n");
    // Set up store operation
    int store_idx = lsq.add_entry(true, 101);  // STORE with ROB index 101
    printf("Created STORE in LSQ at index %d\n", store_idx);
    
    // Set up and resolve dependencies
    lsq.set_base_tag(store_idx, 22);
    lsq.set_offset_tag(store_idx, 23);
    lsq.set_data_tag(store_idx, 24);
    lsq.update_tag(22, 0x200);   // Base address
    lsq.update_tag(23, 0x8);     // Offset
    lsq.update_tag(24, 0x42);    // Data to store
    
    printf("\nLSQ state after setting up STORE:\n");
    lsq.display_status();
    
    // Try to issue to MemFU
    if(mem_fu.can_accept()) {
        printf("Issuing STORE to MemFU\n");
        mem_fu.issue(store_idx);
    }
    
    printf("\nTest 5: Multiple Operations\n");
    // Execute several more cycles
    for(int i = 0; i < 4; i++) {
        printf("\nExecuting cycle %d:\n", i+1);
        mem_fu.execute();
        mem_fu.display_status();
    }

    printf("\nTest 6: Memory Value Verification\n");
    // Read back stored value
    uint32_t read_val = mem_fu.read_memory(0x208);  // Address from store
    printf("Value at 0x208: 0x%x (Expected: 0x42)\n", read_val);

    printf("\nTest 7: Pipeline Stall Condition\n");
    // Try to issue when pipeline is full
    int new_load_idx = lsq.add_entry(false, 102);
    lsq.set_base_tag(new_load_idx, 25);
    lsq.set_offset_tag(new_load_idx, 26);
    lsq.update_tag(25, 0x300);
    lsq.update_tag(26, 0x0);
    
    // Fill pipeline
    for(int i = 0; i < 3; i++) {
        if(mem_fu.can_accept()) {
            mem_fu.issue(new_load_idx);
        }
    }
    
    printf("\nTrying to issue to full pipeline:\n");
    if(!mem_fu.can_accept()) {
        printf("Correctly detected full pipeline\n");
    }
    mem_fu.display_status();
}

void test_integer_fu() {
    printf("\n=== Testing Integer Function Unit ===\n");
    
    // Create required components
    ControlPredictor predictor;
    IntegerFU int_fu(predictor);

    printf("\nTest 1: Initial State\n");
    int_fu.display_status();

    printf("\nTest 2: Basic Arithmetic\n");
    // Test ADD operation
    printf("Testing ADD operation (0x5 + 0xA):\n");
    int_fu.issue(0x1000, INT_ADD, 0x5, 0xA, 100);  // PC=0x1000, src1=5, src2=10, ROB=100
    IntFUResult add_result = int_fu.execute();
    printf("ADD Result: 0x%x, CC Modified: %d, CC Flags: 0x%x\n",
           add_result.value, add_result.cc_modified, add_result.cc_flags);
    
    // Test SUB operation
    printf("\nTesting SUB operation (0x10 - 0x5):\n");
    int_fu.issue(0x1004, INT_SUB, 0x10, 0x5, 101);
    IntFUResult sub_result = int_fu.execute();
    printf("SUB Result: 0x%x, CC Modified: %d, CC Flags: 0x%x\n",
           sub_result.value, sub_result.cc_modified, sub_result.cc_flags);

    printf("\nTest 3: Branch Operations\n");
    // Test backward branch (taken)
    printf("Testing backward branch (offset=-8):\n");
    predictor.establish_entry(0x2000, PRED_BRANCH, -8);  // Set up predictor
    int_fu.issue(0x2000, BRANCH, 0x4, -8, 102);  // Zero flag set, negative offset
    IntFUResult branch_result = int_fu.execute();
    printf("Branch Result: Target=0x%x, Mispredicted=%d\n",
           branch_result.target, branch_result.mispredicted);
    
    // Test forward branch (not taken)
    printf("\nTesting forward branch (offset=8):\n");
    predictor.establish_entry(0x2004, PRED_BRANCH, 8);
    int_fu.issue(0x2004, BRANCH, 0x1, 8, 103);  // Positive flag set, positive offset
    branch_result = int_fu.execute();
    printf("Branch Result: Target=0x%x, Mispredicted=%d\n",
           branch_result.target, branch_result.mispredicted);

    printf("\nTest 4: JALP Instruction\n");
    int_fu.issue(0x3000, JALP, 0, 16, 104);  // Jump to PC+16
    IntFUResult jalp_result = int_fu.execute();
    printf("JALP Result: Target=0x%x, Return Address=0x%x\n",
           jalp_result.target, jalp_result.value);

    printf("\nTest 5: RET Instruction\n");
    int_fu.issue(0x3004, RET, jalp_result.value, 0, 105);  // Return to saved address
    IntFUResult ret_result = int_fu.execute();
    printf("RET Result: Target=0x%x\n", ret_result.target);

    printf("\nTest 6: Busy Status and Pipeline Control\n");
    // Try to issue while executing
    printf("Checking if can accept new operation: %d\n", int_fu.can_accept());
    if (!int_fu.can_accept()) {
        printf("Correctly detected busy state\n");
    }
    
    // Clear and try again
    int_fu.clear();
    printf("After clear, can accept new operation: %d\n", int_fu.can_accept());

    printf("\nTest 7: Condition Code Generation\n");
    // Test zero result
    printf("Testing zero result:\n");
    int_fu.issue(0x4000, INT_SUB, 0x5, 0x5, 106);
    IntFUResult zero_result = int_fu.execute();
    printf("Zero Result CC Flags: 0x%x (Expected: 0x4)\n", zero_result.cc_flags);
    
    // Test negative result
    printf("\nTesting negative result:\n");
    int_fu.issue(0x4004, INT_SUB, 0x5, 0xA, 107);
    IntFUResult neg_result = int_fu.execute();
    printf("Negative Result CC Flags: 0x%x (Expected: 0x2)\n", neg_result.cc_flags);
    
    // Test positive result
    printf("\nTesting positive result:\n");
    int_fu.issue(0x4008, INT_ADD, 0x5, 0x5, 108);
    IntFUResult pos_result = int_fu.execute();
    printf("Positive Result CC Flags: 0x%x (Expected: 0x1)\n", pos_result.cc_flags);
}

void test_rob();  // Existing function
void test_register_manager();  // New function

int main() {
    test_rob();
    test_register_manager();
    test_control_predictor();
    test_lsq();
    test_memory_fu();
    test_integer_fu();
    return 0;
}