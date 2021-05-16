[Jarrett Parker] , [JWP5716@psu.edu]
[Stephen Galucy] , [sag83@psu.edu]
[Benjamin Medoff] , [brm5414@psu.edu]




@Test1: [1], [1000], [6.133]
@Test2: [2], [1000], [10.854]
@Test3: [4], [1000], [20.663]
@Test4: [8], [1000], [35.201]
@Test5: [16], [1000], [38.385]
@Test6: [32], [1000], [37.454]
@Test7: [8], [100], [35.178]
@Test8: [8], [1000], [33.723]
@Test9: [8], [10000], [35.115]

When running the tests we noticed that as the number of threads increased, the time increased as well. Once we got to 8 threads the time increase was much smaller and hit a plateau at 16 threads. The lowest time for our 32 thread test was on average about 39us but the lowest one we encountered was 37us. We believe that as the number of threads increases, so does the amount of time spent waiting. When running the tests on different buffer sizes, we noticed the times did not vary by much. Although the times were almost identical for all of the buffer sizes, we did have an outlier for the 1000B buffer size with 33us. We expected the times for the larger buffers to be smaller since there would be less time waiting for the buffer to have available space.

