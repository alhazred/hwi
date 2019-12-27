#ifdef	__cplusplus
extern "C" {
#endif

#include <libdevinfo.h> 
#include <pcidb.h>

#define VIDEO 1
#define NET   2

#define RS "\x1B[0m"
#define BD  "\033[36m\033[1m"

struct link {
	long l_id;
	struct link *l_next;
	void *l_ptr;
};

struct pchip {
	struct link p_link;
	int p_nvcpu;
	struct link *p_vcpus;
};

struct vcpu {
	struct link v_link;
	struct link v_link_pchip;
	struct pchip *v_pchip;
	char *v_state;
	char *v_cpu_type;
	char *v_fpu_type;
	long v_clock_mhz;
	long v_pchip_id;
	long v_model;
	long v_family;
	long v_step;
	char *v_vendor;
	char *v_brand;
};

static int memslots;
static int num_dsk;
static int type;
static int num_eth;
static int devfs_entry(di_node_t node, di_minor_t minor, void *arg);
static pcidb_hdl_t *pcidb;

void err_fatal(char *s);
void sys_info();
void cpu_info();
void mem_info();
void pci_info(int type);
void drives_info();

#ifdef	__cplusplus
}
#endif
