/*
 * Based on arch/arm/mm/copypage.c
 *
 * Copyright (C) 2002 Deep Blue Solutions Ltd, All Rights Reserved.
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/mm.h>

#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/rkp.h>
void __cpu_copy_user_page(void *kto, const void *kfrom, unsigned long vaddr)
{
	struct page *page = virt_to_page(kto);
	if(rkp_paIsManaged(virt_to_phys(kto))){
		rkp_copy_page(kto,kfrom,PAGE_SIZE);
	}else{
		copy_page(kto, kfrom);
	}
	flush_dcache_page(page);
}
EXPORT_SYMBOL_GPL(__cpu_copy_user_page);

void __cpu_clear_user_page(void *kaddr, unsigned long vaddr)
{
	if(rkp_paIsManaged(virt_to_phys(kaddr))){
		rkp_clear_page(kaddr);
		return;
	}
	clear_page(kaddr);
}
EXPORT_SYMBOL_GPL(__cpu_clear_user_page);
