#ifndef __LOADER_H__
#define __LOADER_H__

#include "Types.h"
#include "Task.h"

#define elf64_addr		unsigned long
#define elf64_off		unsigned long
#define elf64_half		unsigned short int
#define elf64_word		unsigned int
#define elf64_sword		int
#define elf64_xword		unsigned long
#define elf64_sxword	long


// e_ident[] index
#define EI_MAG0			0
#define EI_MAG1			1
#define EI_MAG2			2
#define EI_MAG3			3
#define EI_CLASS		4
#define EI_DATA			5
#define EI_VERSION		6
#define EI_OSABI		7
#define EI_ABIVERSION	8
#define EI_PAD			9
#define EI_NIDENT		16

// e_ident[EI_MAGX]
#define ELFMAG0			0x7F
#define ELFMAG1			'E'
#define ELFMAG2			'L'
#define ELFMAG3			'F'

// e_ident[EI_CLASS]
#define ELFCLASSNONE	0
#define ELFCLASS32		1
#define ELFCLASS64		2

// e_ident[EI_DATA]
#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2

// e_ident[OSABI]
#define ELFOSABI_NONE		0
#define ELFOSABI_HPUX		1
#define ELFOSABI_NETBSD		2
#define ELFOSABI_LINUX		3
#define ELFOSABI_SOLARIS	6
#define ELFOSABI_AIX		7
#define ELFOSABI_FREEBSD	9

// e_type
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOOS		0xFE00
#define ET_HIOS		0xFEFF
#define ET_LOPROC	0xFF00
#define ET_HIPROC	0xFFFF

// e_machine
#define EM_NONE		0
#define EM_M32		1
#define EM_SPARC	2
#define EM_386		3
#define EM_PPC		20
#define EM_PPC64	21
#define EM_ARM		40
#define EM_IA_64	50
#define EM_X86_64	62
#define EM_AVR		83
#define EM_AVR32	185
#define EM_CUDA		190

// special section index
#define SHN_UNDEF		0
#define SHN_LOERSERVE	0xFF00
#define SHN_LOPROC		0xFF00
#define SHN_HIPROC		0xFF1F
#define SHN_LOOS		0xFF20
#define SHN_HIOS		0xFF3F
#define SHN_ABS			0xFFF1
#define SHN_COMMON		0xFFF2
#define SHN_XINDEX		0xFFFF
#define SHN_HIRESERVE	0xFFFF

// sh_type
#define SHT_NULL		0
#define SHT_PROGBITS	1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOOS		0x60000000
#define SHT_HIOS		0x6FFFFFFF
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7FFFFFFF
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xFFFFFFFF

// sh_flags
#define SHF_WRITE		1
#define SHF_ALLOC		2
#define SHF_EXECINSTR	4
#define SHF_MASKOS		0x0FF00000
#define SHF_MASKPROC	0xF0000000

// relocation type
#define R_X86_64_NONE		0
#define R_X86_64_64			1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4
#define R_X86_64_COPY		5
#define R_X86_64_CLOB_DAT	6
#define R_X86_64_JUMP_SLOT	7
#define R_X86_64_RELATIVE	8
#define R_X86_64_GOTPCREL	9
#define R_X86_64_32			10
#define R_X86_64_32S		11
#define R_X86_64_16			12
#define R_X86_64_PC16		13
#define R_X86_64_8			14
#define R_X86_64_PC8		15
#define R_X86_64_DPTMOD64	16
#define R_X86_64_DTPOFF64	17
#define R_X86_64_TPOFF64	18
#define R_X86_64_TLSGD		19
#define R_X86_64_TLSLD		20
#define R_X86_64_DTPOFF32	21
#define R_X86_64_GOTTPOFF	22
#define R_X86_64_TPOFF32	23
#define R_X86_64_PC64		24
#define R_X86_64_GOTOFF64	25
#define R_X86_64_GOTPC32	26
#define R_X86_64_SIZE32		32
#define R_X86_64_SIZE64		33

// macro function
#define RELOCATION_UPPER32(x)	((x) >> 32)
#define RELOCATION_LOWER32(x)	((x) & 0xFFFFFFFF)


// structure

#pragma pack(push, 1)

// elf header
typedef struct
{
	unsigned char e_ident[16];
	elf64_half e_type;
	elf64_half e_machine;
	elf64_word e_version;
	elf64_addr e_entry;
	elf64_off e_phoff;
	elf64_off e_shoff;
	elf64_word e_flags;
	elf64_half e_ehsize;
	elf64_half e_phentsize;
	elf64_half e_phnum;
	elf64_half e_shentsize;
	elf64_half e_shnum;
	elf64_half e_shstrndx;
} elf64_ehdr;

// elf64 section header
typedef struct
{
	elf64_word sh_name;
	elf64_word sh_type;
	elf64_xword sh_flags;
	elf64_addr sh_addr;
	elf64_off sh_offset;
	elf64_xword sh_size;
	elf64_word sh_link;
	elf64_word sh_info;
	elf64_xword sh_addralign;
	elf64_xword sh_entsize;
} elf64_shdr;

// elf64 symbol table entry
typedef struct
{
	elf64_word st_name;
	unsigned char st_info;
	unsigned char st_other;
	elf64_half st_shndx;
	elf64_addr st_value;
	elf64_xword st_size;
} elf64_sym;

// elf64 relocation entry (SHT_REL type)
typedef struct
{
	elf64_addr r_offset;
	elf64_xword r_info;
} elf64_rel;

// elf64 relocation entry (SHT_RELA type)
typedef struct
{
	elf64_addr r_offset;
	elf64_xword r_info;
	elf64_sxword r_addend;
} elf64_rela;

#pragma pack(pop)


// functions
QWORD kExecuteProgram(const char* pcFileName, const char* pcArgumentString,
		BYTE bAffinity);
static BOOL kLoadProgramAndRelocation(BYTE* pbFileBuffer,
		QWORD* pqwApplicationMemoryAddress, QWORD* pqwApplicationMemorySize,
		QWORD* pqwEntryPointAddress);
static BOOL kRelocation(BYTE* pbFileBuffer);
static void kAddArgumentStringToTask(TCB* pstTask, const char* pcArgumentString);
QWORD kCreateThread(QWORD qwEntryPoint, QWORD qwArgument, BYTE bAffinity,
		QWORD qwExitFunction);


#endif
