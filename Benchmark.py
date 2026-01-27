import subprocess
import random
import string
import os
import time
import re
import sys
import argparse

# --- CONFIGURATION ---
DEFAULT_ROWS = 50000
DB_EXE = "./TetoDB" if os.name != 'nt' else "TetoDB.exe"
DB_NAME_PREFIX = "bench_db"

def generate_random_string(length=16):
    letters = string.ascii_letters
    return '"' + ''.join(random.choice(letters) for i in range(length)) + '"'

def generate_dataset(num_rows):
    print(f"Pre-generating data for {num_rows} rows in memory...")
    data = []
    ids = list(range(1, num_rows + 1))
    random.shuffle(ids)
    for uid in ids:
        data.append((uid, generate_random_string()))
    return data

def create_script(filename, table_name, use_index, dataset, target_id):
    print(f"\nWriting script {filename}...")
    
    with open(filename, "w") as f:
        # 1. Create Table
        idx_flag = "1" if use_index else "0"
        f.write(f"create table {table_name} id int {idx_flag} val char 32\n")
        
        num_rows = len(dataset)
        # Commit every 5% of rows
        commit_step = max(5000, num_rows // 20)
        # Update progress bar every 5%
        progress_step = max(1, num_rows // 20)

        # 2. Write Inserts
        for i, (uid, val) in enumerate(dataset):
            f.write(f"insert into {table_name} {uid} {val}\n")
            
            # Progress Bar (Generation)
            if (i + 1) % progress_step == 0:
                percent = ((i + 1) / num_rows) * 100
                sys.stdout.write(f"\r   [Generating: {int(percent)}%] {i+1}/{num_rows} cmds written")
                sys.stdout.flush()

            if (i + 1) % commit_step == 0:
                f.write(".commit\n")
        
        print("") # Newline after progress bar

        # 3. Final Commit
        f.write(".commit\n")
        
        # 4. TEST SELECT (Point Lookup)
        f.write(f"select from {table_name} where id {target_id} {target_id}\n")
        
        # 5. TEST DELETE (Range Delete)
        # We delete a range of 100 IDs to test range-deletion speed
        del_start = 100
        del_end = 200
        f.write(f"delete from {table_name} where id {del_start} {del_end}\n")
        
        f.write(".exit\n")

def run_test(script_name, target_id, expected_val, total_rows):
    # Cleanup old DB files
    for ext in [".db", ".teto", ".btree"]:
        try:
            files = [f for f in os.listdir('.') if f.startswith(DB_NAME_PREFIX) and f.endswith(ext)]
            for f in files: os.remove(f)
        except: pass

    print(f"\nRunning TetoDB with {script_name}...")
    
    start_time = time.time()
    
    process = subprocess.Popen(
        [DB_EXE, DB_NAME_PREFIX, script_name], 
        stdout=subprocess.PIPE, 
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1 
    )

    # Metrics
    insert_times = []
    select_time = 0.0
    delete_time = 0.0
    
    # Verification
    found_target_id = False
    found_correct_val = False
    actual_val_found = "None"
    
    # State Machine for Parsing Output
    # We need to know WHICH command the timing belongs to.
    # We look for the status message immediately preceding the timing.
    last_command_type = "UNKNOWN" 
    
    # Progress Bar Variables
    inserts_done = 0
    update_interval = max(1, total_rows // 20) 
    
    print("--- EXECUTION PROGRESS ---")

    for line in process.stdout:
        # 1. Detect Command Types
        if "row inserted" in line:
            last_command_type = "INSERT"
            inserts_done += 1
            # Live Progress Bar
            if inserts_done % update_interval == 0:
                elapsed = time.time() - start_time
                percent = (inserts_done / total_rows) * 100
                if elapsed > 0 and inserts_done > 0:
                    rows_per_sec = inserts_done / elapsed
                    eta = (total_rows - inserts_done) / rows_per_sec
                    print(f"   [Running: {int(percent)}%] {inserts_done}/{total_rows} inserts | ETA: {eta:.1f}s")

        elif "rows in set" in line:
            last_command_type = "SELECT"
        
        elif "Deleted" in line and "rows" in line:
            last_command_type = "DELETE"

        # 2. Verification Logic (Inspect Select Output)
        elif line.startswith("|"):
            if str(target_id) in line:
                print(f"\n   RESULT: {line.strip()}")
                found_target_id = True
                clean_expected = expected_val.replace('"', '')
                if clean_expected in line:
                    found_correct_val = True
                else:
                    parts = line.split("|")
                    if len(parts) > 2: actual_val_found = parts[2].strip()

        # 3. Capture Timing
        match = re.search(r"\((\d+\.\d+) (ms|us)\)", line)
        if match:
            val = float(match.group(1))
            unit = match.group(2)
            if unit == "us": val /= 1000.0
            
            # Assign timing to the correct bucket
            if last_command_type == "INSERT":
                insert_times.append(val)
            elif last_command_type == "SELECT":
                select_time = val
            elif last_command_type == "DELETE":
                delete_time = val
            
            # Reset state
            last_command_type = "UNKNOWN"
            
    print("--------------------------")
    process.wait()
    total_wall_time = time.time() - start_time
    
    # Calculate Stats
    avg_insert = sum(insert_times) / len(insert_times) if insert_times else 0

    if found_correct_val: status = "PASSED"
    elif found_target_id: status = f"FAILED (Expected {expected_val}, Got {actual_val_found})"
    else: status = "FAILED (Target ID Not Found)"

    print(f"Status: {status}\n")
        
    return avg_insert, select_time, delete_time, status

def main():
    if not os.path.exists(DB_EXE):
        print(f"Error: {DB_EXE} not found.")
        return

    parser = argparse.ArgumentParser()
    parser.add_argument("rows", nargs="?", type=int, default=DEFAULT_ROWS)
    args = parser.parse_args()

    num_rows = args.rows
    target_id = int(num_rows * 0.8) 

    dataset = generate_dataset(num_rows)
    
    expected_val = "UNKNOWN"
    for uid, val in dataset:
        if uid == target_id:
            expected_val = val
            break
            
    print("=" * 60)
    print(f"BENCHMARK: {num_rows} ROWS | TARGET ID: {target_id}")
    print("=" * 60)

    create_script("bench_no_index.txt", "table_slow", False, dataset, target_id)
    create_script("bench_with_index.txt", "table_fast", True, dataset, target_id)
    
    print("=" * 60)
    
    # Run Tests
    print(">>> BENCHMARK 1: NO INDEX (Linear Scan)")
    ins_no, sel_no, del_no, stat_no = run_test("bench_no_index.txt", target_id, expected_val, num_rows)

    print(">>> BENCHMARK 2: WITH INDEX (B-Tree)")
    ins_idx, sel_idx, del_idx, stat_idx = run_test("bench_with_index.txt", target_id, expected_val, num_rows)
    
    print("=" * 80)
    print(f"FINAL RESULTS ({num_rows} Rows)")
    print("=" * 80)
    
    if stat_no != "PASSED" or stat_idx != "PASSED":
        print("WARNING: ONE OR MORE TESTS FAILED CORRECTNESS CHECK!")
        print("-" * 80)

    # Helper for speedup strings
    def get_speedup(slow, fast):
        if fast == 0: return "Infinite"
        return f"{slow/fast:.1f}x"

    print(f"{'Metric':<20} | {'No Index (Linear)':<20} | {'With Index (B-Tree)':<22} | {'Comparison'}")
    print("-" * 90)
    
    # 1. INSERT Comparison
    print(f"{'Avg Insert Time':<20} | {ins_no:.4f} ms{'':<11} | {ins_idx:.4f} ms{'':<13} | Index Overhead: {ins_idx - ins_no:.4f} ms")
    
    # 2. SELECT Comparison
    print(f"{'SELECT Time':<20} | {sel_no:.4f} ms{'':<11} | {sel_idx:.4f} ms{'':<13} | Speedup: {get_speedup(sel_no, sel_idx)}")

    # 3. DELETE Comparison
    print(f"{'DELETE Time':<20} | {del_no:.4f} ms{'':<11} | {del_idx:.4f} ms{'':<13} | Speedup: {get_speedup(del_no, del_idx)}")
    print("-" * 90)

if __name__ == "__main__":
    main()