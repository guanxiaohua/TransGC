TransGC
=======

This is a previous work I've done on hotspot, the name TransGC meaning transactional GC. It conatains the hack that I made to enable hotspot to group objects into different transactional areas so that we can reclaim the space once the transaction is over. The implementation is complete, but unfortunately the overhead is not acceptable. It runs with real benchmarks like specj2004, but the performance is not as good as expected, that's why it's not published into a paper yet. But I would like to still share this code in case anyone is interested.
