# seed generator / scraper
one interesting part of the approach will require scraping samples (small images, PEs, etc)
or select switches of the generator to cover different kind of files, we can possibly have here
a small bash/python script to deliver different switches at each pass.

Take as an example UPX, we have different compression levels that can be tested, and depending
on the kind of file (PE, ELF...)  we are passing, we have different options available
(strip relocs/resources...).

We have a script defining how this guy gets called, and the command line info will be passed down to the actual fuzzer for each run (pipe????) everytime we complete a cycle, we go on and test different input seeds
