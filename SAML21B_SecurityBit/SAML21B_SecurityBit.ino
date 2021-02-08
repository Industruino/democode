void setup()
{
  if (!setSecurityBit())
  {
    // ERROR: can't set security bit

    // ...
  }

  // ...
}

void loop()
{
  // ...
}

bool setSecurityBit()
{
  if (!SUPC->BOD33.bit.ENABLE)
    return false;

  if (NVMCTRL->STATUS.bit.SB)
    return true;

  while (!NVMCTRL->INTFLAG.bit.READY);

  uint32_t nvmCtrlCtrlBReg = NVMCTRL->CTRLB.reg;

  NVMCTRL->CTRLB.bit.CACHEDIS = 1;

  NVMCTRL->STATUS.reg = NVMCTRL_STATUS_NVME | NVMCTRL_STATUS_LOCKE | NVMCTRL_STATUS_PROGE;

  NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_SSB | NVMCTRL_CTRLA_CMDEX_KEY;               

  while (!NVMCTRL->INTFLAG.bit.READY);

  bool returnValue = !(NVMCTRL->STATUS.bit.NVME | NVMCTRL->STATUS.bit.LOCKE | NVMCTRL->STATUS.bit.PROGE);

  NVMCTRL->CTRLB.reg = nvmCtrlCtrlBReg;

  return returnValue;
}
