SECTIONS{
  /* force page alignment let's fuzz faster... */
.fuzzables : {
  . = ALIGN(4096);
  *(.fuzzables)

}
}
