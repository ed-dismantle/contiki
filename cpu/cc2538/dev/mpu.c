/*
* * Copyright (c) 2015, Dismantle Engineering Limited.
* * All rights reserved.
* *
* * Author: Ed Robbins <ed@dismantleengineering.co.uk>
*/

/* Driver for cortex M3 Memory Protection Unit */

#include <mpu.h>
#include <stdio.h>
#include <inttypes.h>

uint8_t is_power_of_two(uint32_t n);
uint8_t cm3_mpu_encode_size(uint32_t size);

#define SHCSR_ADDR 0xE000ED24 /* System Handler Control and State Register */
#define MMFR_ADDR 0xE000ED34 /* Memory manage fault register */

/* Provides access to the MPU registers */
static MPU_Type *mpu = (MPU_Type*)MPU_BASE;

uint8_t
cm3_mpu_init(void) {
  uint32_t *shcsr = (uint32_t*)SHCSR_ADDR;
  uint32_t RASR;
  uint8_t i;

  /* Check the MPU type register has the expected value */
  if (mpu->TYPE != 0x00000800) goto fail;

  /* Put in dummy settings for all of the regions.
   * Turn on all permissions, and disable every region */
  RASR = MPU_RASR_XN_Msk | (0b011 << MPU_RASR_AP_Pos)
          | MPU_RASR_S_Msk | MPU_RASR_C_Msk
          | (14 << MPU_RASR_SIZE_Pos);
  for (i = 0; i < 8; i++) {
    mpu->RNR = i;
    mpu->RASR = RASR;
  }

  /* Enable the MPU fault handler */
  *shcsr |= 1 << 16;

  /* Enable the MPU */
  cm3_mpu_disable_in_interrupt_handlers();
  cm3_mpu_enable_background_region();
  cm3_mpu_enable();

  return SUCCESS;
fail:
  return FAIL;
}

void
cm3_mpu_fault_handler(void) {
  uint32_t *mmfr = (uint32_t*)MMFR_ADDR;
  printf("Memory fault at 0x%" PRIx32 "\n", (unsigned int)*mmfr);
  while(1);
}

uint8_t
cm3_mpu_set_region_range(uint32_t region, uint32_t start, uint32_t size) {
  uint32_t size_bits, RASR;

  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* region size must be a power of 2, greater than 16 */
  if (size < 32 || !is_power_of_two(size)) goto fail;

  /* we are also enforcing that size =< 32768, but this is not a hardware
   * constraint (we only have 32K of RAM so it's just more efficient */
  if (size > 32768) goto fail;

  /* region start address must be aligned to size */
  if (start % size != 0) goto fail;

  /* All seems ok so get an encoding for the size */
  size_bits = cm3_mpu_encode_size(size);

  /* Select the region */
  mpu->RNR = region;

  /* Set base address */
  mpu->RBAR = start;

  /* Set size */
  RASR = mpu->RASR;
  RASR &= ~MPU_RASR_SIZE_Msk;
  RASR |= size_bits << MPU_RASR_SIZE_Pos;
  mpu->RASR = RASR;

  return SUCCESS;
fail:
  return FAIL;
}

uint8_t
cm3_mpu_set_region_readable_writable(uint32_t region, uint32_t readable, uint32_t writeable) {
  uint32_t access_permissions, RASR;

  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* Select the region */
  mpu->RNR = region;

  /* Set access permissions */
  if (readable && writeable) access_permissions = 0b011;
  else if (readable) access_permissions = 0b110;
  else access_permissions = 0b000;

  /* Write to access permissions bits of RAS Register */
  RASR = mpu->RASR;
  RASR &= ~MPU_RASR_AP_Msk;
  RASR |= access_permissions << MPU_RASR_AP_Pos;
  mpu->RASR = RASR;

  return SUCCESS;
fail:
  return FAIL;
}

uint8_t
cm3_mpu_enable_region(uint32_t region) {
  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* Select the region */
  mpu->RNR = region;

  /* Enable the region */
  mpu->RASR |= MPU_RASR_ENABLE_Msk;

  return SUCCESS;
fail:
  return FAIL;
}

uint8_t
cm3_mpu_disable_region(uint32_t region) {
  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* Select the region */
  mpu->RNR = region;

  /* Disable the region */
  mpu->RASR &= ~MPU_RASR_ENABLE_Msk;

  return SUCCESS;
fail:
  return FAIL;
}

uint8_t
cm3_mpu_enable_subregion(uint32_t region, uint32_t subregion) {
  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* There can only be 8 subregions */
  if (region > 7) goto fail;

  /* Select the region */
  mpu->RNR = region;

  /* Enable the specified subregion */
  mpu->RASR &= ~(1 << (MPU_RASR_SRD_Pos + subregion));

  return SUCCESS;
fail:
  return FAIL;
}

uint8_t
cm3_mpu_disable_subregion(uint32_t region, uint32_t subregion) {
  /* There are only 8 regions */
  if (region > 7) goto fail;

  /* There can only be 8 subregions */
  if (region > 7) goto fail;

  /* Select the region */
  mpu->RNR = region;

  /* Disable the specified subregion */
  mpu->RASR |= 1 << (MPU_RASR_SRD_Pos + subregion);

  return SUCCESS;
fail:
  return FAIL;
}

void
cm3_mpu_enable(void) {
  mpu->CTRL |= MPU_CTRL_ENABLE_Msk;
}

void
cm3_mpu_disable(void) {
  mpu->CTRL &= !MPU_CTRL_ENABLE_Msk;
}

void
cm3_mpu_enable_background_region(void) {
  mpu->CTRL |= MPU_CTRL_PRIVDEFENA_Msk;
}

void
cm3_mpu_disable_background_region(void) {
  mpu->CTRL &= !MPU_CTRL_PRIVDEFENA_Msk;
}

void
cm3_mpu_enable_in_interrupt_handlers(void) {
  mpu->CTRL |= MPU_CTRL_HFNMIENA_Msk;
}

void
cm3_mpu_disable_in_interrupt_handlers(void) {
  mpu->CTRL &= !MPU_CTRL_HFNMIENA_Msk;
}

uint8_t
is_power_of_two(uint32_t n) {
  return n && (! (n & (n-1)) );
}

/* I considered various ways to do this, even consulting hackers delight, but
 * in the end this seemed like it would probably be the most efficient,
 * especially if we restrict the size to being =< 32KB (we only have that much
 * RAM on the cc2538. This would need to be expanded to support the full
 * possibilities of the MPU (where size can be up to 4GB) */
uint8_t
cm3_mpu_encode_size(uint32_t size) {
  switch (size) {
    case 32:
      return 4;
    case 64:
      return 5;
    case 128:
      return 6;
    case 256:
      return 7;
    case 512:
      return 8;
    case 1024:
      return 9;
    case 2048:
      return 10;
    case 4096:
      return 11;
    case 8192:
      return 12;
    case 16384:
      return 13;
    case 32768:
      return 14;
  }
  /* We should never reach this */
  return FAIL;
}

