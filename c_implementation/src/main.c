/**
@author     Matthew Spero (Watercycle@email.com)
@date       May 1, 2015
@brief      LC-3 Simulator

Objective:  Implement an LC-3 Simulator

Compiler: on Windows using GNU GCC v4.9.2 with flags
    "-Wall -ansi -pedantic -std=c99 -lm".
*/

#include <stdio.h>  // fgets, file, fopen, ...
#include <stdlib.h> // exit
#include <string.h> // memset, strlen, ...
#include <ctype.h>  // isdigit, tolower, ...

//-----------------------Setup-----------------------------//
//---------------------------------------------------------//

typedef short Word;				  // type that represents a word of LC3 mem (the value)
typedef unsigned short Address;   // type that represents an LC3 address

#define MEMLEN 65536 // 2^16 mem locations
#define NREG 8		 // number of registers

typedef struct
{
	Word mem[MEMLEN];	 // For storage in mem
	Word reg[NREG];      // For math operations
	Address pc;          // Program Counter
	int running;         // running = 1 iff CPU is executing instructions
	Word ir;             // Instruction Register
	char cc;			 // condition code
} CPU;

//----------------Many Forward Declarations----------------//
//---------------------------------------------------------//

int main(int argc, char *argv[]);
void initialize_control_unit(CPU *cpu, FILE* datafile);
void initialize_mem(CPU *cpu, FILE* datafile);
FILE *get_datafile(int argc, char *argv[]);

void dump_mem(CPU *cpu);
void dump_registers(CPU *cpu);
void dump_control_unit(CPU *cpu);
void dump_all(CPU *cpu);

Word fetch_instruction(FILE *datafile, Word *instr, Word *param_assigned);
int read_execute_command(CPU *cpu);
int execute_command(char cmd_char, char* cmd_line, CPU *cpu);
void help_message(void);
void many_instruction_cycles(int nbr_cycles, CPU *cpu);
void one_instruction_cycle(CPU *cpu);
void update_condition_code(CPU *cpu, Word value);

void set_value(char* command_line, CPU *cpu);
void set_address_value(Address mem_address, Word value, CPU *cpu);
void set_register_value(Address reg_num, Word value, CPU *cpu);
void goto_address_location(char *command_line, CPU *cpu);

void instr_ADD (CPU *cpu);
void instr_AND (CPU *cpu);
void instr_BR  (CPU *cpu);
void instr_err (CPU *cpu);
void instr_JMP (CPU *cpu);
void instr_JSR (CPU *cpu);
void instr_LD  (CPU *cpu);
void instr_LDI (CPU *cpu);
void instr_LDR (CPU *cpu);
void instr_LEA (CPU *cpu);
void instr_NOT (CPU *cpu);
void instr_RTI (CPU *cpu);
void instr_ST  (CPU *cpu);
void instr_STI (CPU *cpu);
void instr_STR (CPU *cpu);
void instr_TRAP(CPU *cpu);

void trap_getchar(CPU *cpu);
void trap_out(CPU *cpu);
void trap_puts(CPU *cpu);
void trap_input(CPU *cpu);
void trap_halt(CPU *cpu);

char* lowercase(char* str);
Word bit_select_us(Word value, unsigned left, unsigned right);
Word bit_select_s(Word value, unsigned left, unsigned right);
void print_binary(unsigned int x);

//---------------------------------------------------------//

// Initializes the cpu to begin the read and execute process and
// provides context in the console.
int main(int argc, char *argv[])
{
	printf("LC3 Simulator: CS 350 Lab 8 / Final Project ~Matthew Spero\n");

	CPU cpu;
	FILE *datafile = get_datafile(argc, argv); //the file being simulated

	initialize_control_unit(&cpu, datafile); // setup the origin
	initialize_mem(&cpu, datafile); //load the datafile into mem
	dump_all(&cpu);

	fclose(datafile); // finished with loading the file

	printf("\nBeginning execution; type h for help\n> ");

	//commands will be continuously requested until 'q' is entered to quit
	int done = read_execute_command(&cpu);
	while (!done)
	{
		printf("> ");
		done = read_execute_command(&cpu);
	}

	return EXIT_SUCCESS;
}

// Initializes the cpu's control unit to its default values and
// determines the origin of the program counter.
void initialize_control_unit(CPU *cpu, FILE *datafile)
{
	Word origin, params;
	if(!fetch_instruction(datafile, &origin, &params) || params < 1) {
		printf("Error: Couldn't read origin; quitting");
		exit(EXIT_FAILURE);
	}

	cpu->pc = origin;
	cpu->ir = 0;
	cpu->cc = 'Z';
	cpu->running = 1;
	memset(cpu->reg, 0, sizeof(cpu->reg)); //zero out the registers
}

// Zeros out the cpu's mem and then initialiazes the datafile into mem
// with a few validation checks.  And, negative addresses will wrap around
// (i.e. 0x8000 to 0xffff = -32,768 to -1).
void initialize_mem(CPU *cpu, FILE *datafile)
{
	memset(cpu->mem, 0, sizeof(cpu->mem)); //zero out the mem

	Word value_read, params; //variables we will the datafile into
	int mem_loc = cpu->pc; //use a temporary variable so we don't modify our origin

	while (fetch_instruction(datafile, &value_read, &params)) {
		if (mem_loc > 0xffff) mem_loc = 0x0000; //wrap around addresses
		if (params > 0) cpu->mem[mem_loc++] = value_read; //save the instruction to mem
	}
}

// Returns the newly opened datafile based on the file specified when
// the program is first ran.  Otherwise, the program will terminate upon
// failure to open the datafile.
FILE *get_datafile(int argc, char *argv[])
{
	char *datafile_name = argv[1] ? argv[1] : "default.hex";
    printf("<-----Loading file '%s'----->\n\n", datafile_name);

	FILE *datafile = fopen(datafile_name, "r");
    if (!datafile)
    {
        printf("Failed to open '%s', exiting program.\n", datafile_name);
        exit(EXIT_FAILURE);
    }

	return datafile;
}

// Prints out the control unit (the general CPU properties) of the
// given CPU along with all of its registers.
void dump_control_unit(CPU *cpu)
{
	printf("Control Unit:");
	printf("\nPC = x%04hx    IR = x%04hx    CC = %c    RUNNING: %i",
		cpu->pc, cpu-> ir, cpu->cc, cpu->running);

	dump_registers(cpu);
	printf("\n\n");
}

// Prints out all of the instructions and their decimal value of each
// mem address in use (i.e. != 0) in hex.
void dump_mem(CPU *cpu)
{
	printf("mem:    (addresses x0000 - xFFFF)\n");
	for (int i = 0; i < MEMLEN; i++) {
		int value = cpu->mem[i];
		if (value) printf("x%04hx: x%04hx	%i\n", i, value, value);
	}
}

// Prints all of a CPU's registers in two rows.
void dump_registers(CPU *cpu)
{
	for (int i = 0; i < NREG; i++) {
        if (i % (NREG / 2) == 0) printf("\n"); //break it up into 2 lines
        printf("R%i: x%04hx  %i	", i, cpu->reg[i], cpu->reg[i]);
	}
}

// Prints all information related to a given CPU
void dump_all(CPU *cpu)
{
	dump_control_unit(cpu);
	dump_mem(cpu);
}

// Reads a simulator command from the keyboard ("h", "?", "d", number,
// or empty line) and executes it.  Returns true if it hits end-of-input
// or the quit command is used.  Otherwise return false (continues).
int read_execute_command(CPU *cpu)
{
	char cmd_buffer[128]; // a buffer for commands from the keyboard/input
	fgets(cmd_buffer, sizeof(cmd_buffer), stdin);

	char cmd = '\0'; // the general command
	sscanf(cmd_buffer, "%c", &cmd); //first attempt to read it as a character

	int done = 0; // assume we will continue execution
	if (isdigit(cmd)) // if the first input isa number we execute multi-instruction
	{
		int num_cycles = 0; // cmd might not be the full number (i.e. 5 vs 52)
		sscanf(cmd_buffer, "%i", &num_cycles);

		if (num_cycles > 0) {
			many_instruction_cycles(num_cycles, cpu);
		} else {
			printf("Invalid number of cycles entered; ignoring");
		}
	}
	else // execute a single command
	{
		done = execute_command(cmd, cmd_buffer, cpu);
	};

	return done; //0 continues simulator, 1 ends
}

// Executes specific commands provided from user input / the cmd it is given
int execute_command(char cmd, char* cmd_line, CPU *cpu)
{
	switch (tolower(cmd)) //uppercase allowed
	{
		case '?': /* fallthrough */
		case 'h': help_message(); break;
		case 'd': dump_all(cpu); break;
		case 'q': printf("Quitting program\n");	return 1;
		case 's': set_value(cmd_line, cpu); break;
		case 'g': goto_address_location(cmd_line, cpu); break;
		case '\n': one_instruction_cycle(cpu); break;
		default: printf("Unkown command; ignoring.\n");
	}

	return 0;
}

// Prints standard message for simulator help command ('h' or '?')
void help_message()
{
	printf( "Simulator commands:\n"
			"h or ? to print this help message\n"
			"q to quit\n"
			"d to dump the control unit and mem\n"
			"g [address] to go to the new address location\n"
			"s [address] [value] to set the value of an address location\n"
			"s [rN] [value] to set the value of register N\n"
			"Enter an integer > 0 to execute that many instruction cycles\n"
			"Press return to execute one instruction cycle\n"
			"Note: Addresses and values should be in hex (xNNNN)\n");
}

// Attempts to execute a one cycle instruction several times in a row
void many_instruction_cycles(int nbr_cycles, CPU *cpu)
{
	if (!cpu->running) {
		printf("The CPU is not running, unable to execute multi-instruction set.\n");
		return;
	}

	if (nbr_cycles > 100) {
		printf("'%i' is a large number of executions, "
			"substituting with %i cycles instead.\n", nbr_cycles, 100);
		nbr_cycles = 100;
	}

	int cycles_ran = 0; //keep track of our current cycle
	while (cpu->running && cycles_ran < nbr_cycles) {
		one_instruction_cycle(cpu);
		cycles_ran++;
	}
}

// Executes one instruction cycle (no implementation yet)
void one_instruction_cycle(CPU *cpu)
{
    if (!cpu->running) {
        printf("CPU is not currently running, unable to process instructions.\n");
        return; //prevent the instruction cycle
    }

	if (cpu->pc > 0xffff) cpu->pc = 0x0000; //wrap around instructions

	printf("x%04hx: x%04hx | ", cpu->pc, cpu->mem[cpu->pc]);
	cpu->ir = cpu->mem[cpu->pc++]; // sets current instruction and set pc to the next

	int op_code = bit_select_us(cpu->ir, 15, 12);
	switch (op_code)
	{
		case 0:  instr_BR  (cpu); break;
		case 1:  instr_ADD (cpu); break;
		case 2:  instr_LD  (cpu); break;
		case 3:  instr_ST  (cpu); break;
		case 4:  instr_JSR (cpu); break;
		case 5:  instr_AND (cpu); break;
		case 6:  instr_LDR (cpu); break;
		case 7:  instr_STR (cpu); break;
		case 8:  instr_RTI (cpu); break;
		case 9:  instr_NOT (cpu); break;
		case 10: instr_LDI (cpu); break;
		case 11: instr_STI (cpu); break;
		case 12: instr_JMP (cpu); break;
		case 13: instr_err (cpu); break;
		case 14: instr_LEA (cpu); break;
		case 15: instr_TRAP(cpu); break;
		default: printf("Bad opcode: %i, Ignoring.", op_code);
	}


	printf("\n");
}

// Interprets the command_line input in order to establish the type of 's'
// being used.  Note: "%*[ \n\ts]" = ignore the characters in the brackets.
void set_value(char* command_line, CPU *cpu)
{
	Address address = 0;
	unsigned short value = 0; //values read in as unsigned shorts
	char* safecmd = lowercase(command_line);

	// check whether set is refering to the mem command
	int num_assigned = sscanf(safecmd, "%*[ \n\ts] x%04hx x%04hx", &address, &value);
	if (num_assigned >= 2) {
		set_address_value(address, (Word)value, cpu);
	} else {
		// it wasn't mem, so check whether it is the registery version
		num_assigned = sscanf(safecmd, "%*[ \n\ts] x%04hx x%04hx", &address, &value);
		if (num_assigned >= 2) {
			set_register_value(address, value, cpu);
		} else {
			// no match was found, so the user entered the command incorrectly
			printf("Invalid syntax, use either 's xNNNN xNNNN' or 's rN xNNNN'\n");
		}
	}

	free(safecmd); // bleh C.  I missed my unique_ptr's.
}

// Sets the value of a mem address with bounds checking.
void set_address_value(Address mem_address, Word value, CPU* cpu)
{
	if (mem_address >= 0 && mem_address < MEMLEN) {
		printf("Set mem address %hx to %i\n", mem_address, value);
		cpu->mem[mem_address] = value;
	} else {
		printf("Unable to set value for address %hx, must be within x0000 - xFFFF\n", mem_address);
	}
}

// Sets the value of a register with bounds checking.
void set_register_value(Address reg_num, Word value, CPU* cpu)
{
	if (reg_num >= 0 && reg_num < NREG) {
		printf("Set r%i value to %i\n", reg_num, value);
		cpu->reg[reg_num] = value;
	} else {
		printf("Unable set value for register %i, must be within 0 - %i\n", reg_num, NREG - 1);
	}
}

// Interprets the 'g' command to determine where to move the program
// counter to (and then does so).
void goto_address_location(char *command_line, CPU *cpu)
{
	Address address = 0;
	sscanf(command_line, "%*[ \n\tg] x%04hx", &address);

	if (address >= 0 && address < MEMLEN) {
		printf("Set address to %hx\n", address);
		cpu->pc = address;
		cpu->running = 1;
	} else {
		printf("Unable to go to location %hx, must be within x0000 - xFFFF\n", address);
	}
}

// A helper function used to read in the hex datafiles in a more concise way
Word fetch_instruction(FILE *datafile, Word *instr, Word *param_assigned)
{
	unsigned short value; //values read in as unsigned shorts
	*param_assigned = 0; //assume none
	char line_buffer[256]; //a buffer to read the line into

	//# of parameters assigned
	if (fgets(line_buffer, sizeof(line_buffer), datafile)) {
		*param_assigned = sscanf(line_buffer, "%hx", &value);
		*instr = (Word)value;
		return 1;
	}

	return 0; //reached EOF
}

// Whenever one of the specified LC3 operations is performed that alters the
// condition code based on sign, this function is to be called.
void update_condition_code(CPU *cpu, Word value)
{
	if (value < 0) cpu->cc = 'N';
	else if (value == 0) cpu->cc = 'Z';
	else cpu->cc = 'P';

	printf("; CC = %c", cpu->cc);
}

// This program stores the condition code as a character, so this will help
// utilize numeric values when needed.
Word get_condition_num(CPU *cpu)
{
	switch (cpu->cc)
	{
		case 'N': return 1; // 001
		case 'Z': return 2; // 010
		case 'P': return 4; // 100
		default:  return 0; // shouldn't happen
	}
}

void instr_ADD(CPU *cpu)
{
	int type = bit_select_us(cpu->ir, 5, 5); // encoding type
	int dr = bit_select_us(cpu->ir, 11, 9); // destination register
	int src1 = bit_select_us(cpu->ir, 8, 6);

	if (type == 0) {
		int src2 = bit_select_us(cpu->ir, 2, 0);
		cpu->reg[dr] = cpu->reg[src1] + cpu->reg[src2];

		printf("ADD R%i, R%i, R%i // R%i <-- %i%+i = %i",
			dr, src1, src2, dr, cpu->reg[src1], cpu->reg[src2],
			cpu->reg[src1] + cpu->reg[src2]);
	} else {
		int imm5 = bit_select_s(cpu->ir, 4, 0);
		cpu->reg[dr] = cpu->reg[src1] + imm5;

		printf("ADD R%i, R%i, %i // R%i <-- %i%+i = %i",
			dr, src1, imm5, dr, cpu->reg[src1], imm5, cpu->reg[src1], cpu->reg[dr]);
	}

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_AND(CPU *cpu)
{
	int type = bit_select_us(cpu->ir, 5, 5); //encoding type
	int dr = bit_select_us(cpu->ir, 11, 9); // the Destination Register
	int src1 = bit_select_us(cpu->ir, 8, 6);

	if (type == 0) {
		int src2 = bit_select_us(cpu->ir, 2, 0);
		cpu->reg[dr] = cpu->reg[src1] & cpu->reg[src2];
		printf("AND R%i, R%i, R%i // R%i = R%i & R%i = %i",
			dr, src1, src2, dr, src1, src2, cpu->reg[src1] & cpu->reg[src2]);
	} else {
		int imm5 = bit_select_s(cpu->ir, 4, 0);
		cpu->reg[dr] = cpu->reg[src1] & imm5;
		printf("AND R%i, R%i, %i // R%i = R%i & %i = %i",
			dr, src1, imm5, dr, src1, imm5, cpu->reg[src1] & imm5);
	}

	update_condition_code(cpu, cpu->reg[dr]);
}

// The branch type is chosen by utilizing a bitwise and and the cpu's condition code
void instr_BR(CPU *cpu)
{
	int branch_type = bit_select_us(cpu->ir, 11, 9);
	Word pcoffset = bit_select_s(cpu->ir, 8, 0);
	char branch_msg[32];

	switch (branch_type)
	{
		case 2:	 strcpy(branch_msg, "Z (= 0)");	break;
		case 3:	 strcpy(branch_msg, "PZ (>= 0)"); break;
		case 4:	 strcpy(branch_msg, "N (< 0)");	break;
		case 5:	 strcpy(branch_msg, "NP (!= 0)"); break;
		case 6:  strcpy(branch_msg, "NZ (<= 0)"); break;
		case 7:  strcpy(branch_msg, "BR (unconditional)"); break;
		default: strcpy(branch_msg, "NOP"); break;
	}

	if ((branch_type & get_condition_num(cpu)) != 0) {
		int old_pc = cpu->pc;
		cpu->pc += pcoffset;

		printf("BR: %s is true, CC = %c, new pc = x%04hx%+i = x%04hx",
			branch_msg, cpu->cc, old_pc, pcoffset, cpu->pc);
	} else {
		printf("BR: %s is false! CC = %c, doing nothing", branch_msg, cpu->cc);
	}
}

void instr_err(CPU *cpu)
{
	printf("Unused opcode");
}

void instr_JMP(CPU *cpu)
{
	int base = bit_select_us(cpu->ir, 8, 6);
	cpu->pc = cpu->reg[base];
	printf("JMP, pc = R%i = x%04hx", base, cpu->reg[base]);
}

void instr_JSR(CPU *cpu)
{
	int mode = bit_select_us(cpu->ir, 11, 11);

	cpu->reg[7] = cpu->pc; //save current instructions
	if (mode == 1) {
		int pcoffset = bit_select_s(cpu->ir, 10, 0);
		cpu->pc = cpu->pc + pcoffset;
		printf("JSR pc = PC%+i = x%04hx", pcoffset, cpu->pc + pcoffset);
	} else { //JSRR
		int regNum = bit_select_us(cpu->ir, 8, 6);
		cpu->pc = cpu->reg[regNum]; //pc = contents of register
		printf("JSSR R%i = x%04hx (R7 = x%04hx)", regNum, cpu->reg[regNum], cpu->reg[7]);
	}
}

void instr_LD(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int pcoffset = bit_select_s(cpu->ir, 8, 0);

	cpu->reg[dr] = cpu->mem[cpu->pc + pcoffset];
	printf("LD R%i, %i // R%i = M[PC%+i] = M[x%04hx] = %i",
		dr, pcoffset, dr, pcoffset, cpu->pc + pcoffset, cpu->mem[cpu->pc + pcoffset]);

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_LDI(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int pcoffset = bit_select_s(cpu->ir, 8, 0);

	cpu->reg[dr] = cpu->mem[cpu->mem[pcoffset + cpu->pc]];
	printf("LDI R%i, %hx // R%i = M[ M[x%04hx + x%04hx] ] = x%04hx",
		dr, cpu->pc + pcoffset, dr, pcoffset, cpu->pc, cpu->mem[cpu->mem[pcoffset + cpu->pc]]);

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_LDR(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int base = bit_select_us(cpu->ir, 8, 6);
	int offset = bit_select_s(cpu->ir, 5, 0);

	cpu->reg[dr] = cpu->mem[cpu->reg[base] + offset];
	printf("LDR R%i, x%04hx, x%04hx // R%i = M[ R%i + x%04hx ] = x%04hx",
		dr, base, offset, dr, base, offset, cpu->mem[cpu->reg[base] + offset]);

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_LEA(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int pcoffset = bit_select_s(cpu->ir, 8, 0);

	cpu->reg[dr] = cpu->pc + pcoffset;
	printf("LEA R%i, x%04hx // R%i = x%04hx + x%04hx = x%04hx",
		dr, cpu->pc + pcoffset, dr, cpu->pc, pcoffset, cpu->pc + pcoffset);

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_NOT(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int src = bit_select_us(cpu->ir, 8, 6);

	cpu->reg[dr] = ~cpu->reg[src];
	printf("NOT R%i, R%i // R%i = ~R%i = x%04hx", dr, src, dr, src, ~cpu->reg[src]);

	update_condition_code(cpu, cpu->reg[dr]);
}

void instr_RTI(CPU *cpu)
{
	printf("RTI not supported");
}

void instr_ST(CPU *cpu)
{
	int src = bit_select_us(cpu->ir, 11, 9);
	int pcoffset = bit_select_s(cpu->ir, 8, 0);

	cpu->mem[cpu->pc + pcoffset] = cpu->reg[src];
	printf("ST R%i, %i // M[PC%+i] = M[x%04hx] <-- R%i = %i",
		src, pcoffset, pcoffset, cpu->pc + pcoffset, src, cpu->reg[src]);
}

void instr_STI(CPU *cpu)
{
	int src = bit_select_us(cpu->ir, 11, 9);
	int pcoffset = bit_select_s(cpu->ir, 8, 0);

	cpu->mem[cpu->mem[pcoffset + cpu->pc]] = cpu->reg[src];
	printf("STI R%i, x%04hx // M[ M[x%04hx + %i] ] = R%i = %i",
		src, pcoffset, cpu->pc, pcoffset, src, cpu->reg[src]);
}

void instr_STR(CPU *cpu)
{
	int dr = bit_select_us(cpu->ir, 11, 9);
	int base = bit_select_us(cpu->ir, 8, 6);
	int offset = bit_select_s(cpu->ir, 5, 0);

	cpu->mem[base + offset] = cpu->reg[dr];
	printf("STR R%i, R%i, %i // M[x%04hx%+i] = M[x%04hx%] <-- R%i = %i",
		dr, base, offset, cpu->reg[base], cpu->reg[base] + offset, offset, dr, cpu->reg[dr]);
}

void instr_TRAP(CPU *cpu)
{
	int trap_vector = bit_select_us(cpu->ir, 7, 0);
	switch (trap_vector)
	{
		case 0x20: trap_getchar(cpu); break;
		case 0x21: trap_out(cpu);     break;
		case 0x22: trap_puts(cpu);    break;
		case 0x23: trap_input(cpu);   break;
		case 0x25: trap_halt(cpu);    break;
		default: printf("Trap code x%04hx not supported, halting execution", trap_vector);
			     cpu->running = 0;
	}
}

void trap_getchar(CPU *cpu)
{
	char input;
	scanf("%c", &input);
	cpu->reg[0] = input;
	printf("Read in character '%c'", input);
}

void trap_out(CPU *cpu)
{
	printf("%c", cpu->reg[0]);
}
void trap_puts(CPU *cpu)
{
	Word current_loc = cpu->reg[0];
	while (cpu->mem[current_loc]) printf("%c", cpu->mem[current_loc++]);
}

void trap_input(CPU *cpu)
{
	printf("Enter a Character: ");
	trap_getchar(cpu);
}

void trap_halt(CPU *cpu)
{
	printf("Trap halt reached, halting CPU.");
	cpu->cc = 'P'; //asked by requirements
	cpu->running = 0;
}

// Fills the 'lower' character array with a lowercase version of str and then returns it.
// Note: lower must be freed once it is done being used.
char* lowercase(char* str)
{
	int length = strlen(str);
	char* lower = malloc(sizeof(char) * (length + 1));
	strcpy(lower, str);

	for (int i = 0; i < length; i++) lower[i] = tolower(lower[i]);
	return lower;
}


// Example: 50 in decimal = 0011 0010 in binary.  Calling bit_select_us(50, 4, 1)
// will return 1001 in binary, so 9 in decimal.
Word bit_select_us(Word value, unsigned left, unsigned right)
{
	int number_of_ones = (left - right) + 1;
	int mask = ((1 << number_of_ones) - 1) << right;
	int actual_value = (value & mask) >> right;
	return actual_value;
}

// Example: 50 in decimal = 0011 0010 in binary.  Calling bit_select_s(50, 4, 1)
// will return 0111 in binary, so -9 in decimal.  So, this is a signed selection.
Word bit_select_s(Word value, unsigned left, unsigned right)
{
	if ((value & (1 << abs(right - left))) != 0) {
		return -bit_select_us(~value + 1, left, right); //complement
	} else {
		return bit_select_us(value, left, right);
	}
}

// Another helper function to help debug and interpret operations by
// printing out decimal strings in binary.
void print_binary(unsigned int x)
{
	for (int i = (sizeof(int) * 4) - 1; i >= 0; i--) {
		if ((i + 1) % 4 == 0) putchar(' ');
		(x & (1 << i)) ? putchar('1') : putchar('0');
	}

	printf("\n");
}
