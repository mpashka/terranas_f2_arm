#ifndef __ASM_TLB_H
#define __ASM_TLB_H

/*
 * MIPS doesn't need any special per-pte or per-vma handling, except
 * we need to flush cache for area to be unmapped.
 */
#define tlb_start_vma(tlb, vma)					\
	do {							\
		if (!tlb->fullmm)				\
			flush_cache_range(vma, vma->vm_start, vma->vm_end); \
	}  while (0)
#define tlb_end_vma(tlb, vma) do { } while (0)
#define __tlb_remove_tlb_entry(tlb, ptep, address) do { } while (0)

/*
 * .. because we flush the whole mm when it fills up.
 */
#define tlb_flush(tlb) flush_tlb_mm((tlb)->mm)

#if defined(CONFIG_CPU_R3000) || defined(CONFIG_CPU_TX39XX) || defined(CONFIG_CPU_RLX)
#define ENTRYHI_VPN_SHIFT	(PAGE_SHIFT)
#define ENTRYHI_VPN_MASK	(PAGE_MASK)
#else
#define ENTRYHI_VPN_SHIFT	(PAGE_SHIFT + 1)
#define ENTRYHI_VPN_MASK	(PAGE_MASK << 1)
#endif

#define UNIQUE_ENTRYHI(idx)					\
		((CKSEG0 + ((idx) << ENTRYHI_VPN_SHIFT)) |	\
		 (cpu_has_tlbinv ? MIPS_ENTRYHI_EHINV : 0))

#include <asm-generic/tlb.h>

#endif /* __ASM_TLB_H */
