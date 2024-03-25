So the PFN is correctly being updated, so why is the signal being trigger again?

```bash
paris@paris-pc:~/SAR/PSAR/PSAR$ make
make internal_tests
make[1]: Entering directory '/home/paris/SAR/PSAR/PSAR'
gcc  -I./include src/internal.c tests/internal_tests.c -o internal_tests
make[1]: Leaving directory '/home/paris/SAR/PSAR/PSAR'
./internal_tests

Process 5185 reads first letter of files/file_0 : [H]

Handler caught SIGSEGV - write attempt by process 5185
Faulting address (aligned): 0x7f46bb325000
Before Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 267090
New page mapped at: 0x7f46bb2eb000
Content copied to new page by process 5185
After Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 228578


Handler caught SIGSEGV - write attempt by process 5185
Faulting address (aligned): 0x7f46bb325000
Before Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 228578
New page mapped at: 0x7f46bb2e9000
Content copied to new page by process 5185
After Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 1939164


Handler caught SIGSEGV - write attempt by process 5185
Faulting address (aligned): 0x7f46bb325000
Before Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 1939164
New page mapped at: 0x7f46bb2e7000
Content copied to new page by process 5185
After Update: Virtual address 0x7f46bb325000 -> Physical Frame Number (PFN): 1872133
^Cmake: *** [Makefile:6: run_test] Interrupt
```