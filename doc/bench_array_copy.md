# HostArray iterator plain pointer
benchmark                                                   arg                      status               time(ns)
copy_host_cached_to_host_FixDataHostCached                  {1024}                   IMPRECISE                  51
copy_host_cached_to_host_FixDataHostCached                  {524288}                 ok                      70638
copy_host_cached_to_host_FixDataHostCached                  {1048576}                ok                     269691
copy_host_cached_to_host_FixDataHostCached                  {536870912}              ok                  269852111
copy_host_to_host_cached_FixDataHostCached                  {1024}                   IMPRECISE                  51
copy_host_to_host_cached_FixDataHostCached                  {524288}                 ok                      70129
copy_host_to_host_cached_FixDataHostCached                  {1048576}                ok                     278515
copy_host_to_host_cached_FixDataHostCached                  {536870912}              ok                  265546462
copy_host_to_host_visible_FixDataHostVisible                {1024}                   IMPRECISE                  73
copy_host_to_host_visible_FixDataHostVisible                {524288}                 ok                     115687
copy_host_to_host_visible_FixDataHostVisible                {1048576}                ok                     241346
copy_host_to_host_visible_FixDataHostVisible                {536870912}              ok                  259091546
copy_host_visible_to_host_FixDataHostVisible                {1024}                   ok                      12605
copy_host_visible_to_host_FixDataHostVisible                {524288}                 ok                    5455824
copy_host_visible_to_host_FixDataHostVisible                {1048576}                ok                   10968186
copy_host_visible_to_host_FixDataHostVisible                {536870912}              ok                 8059899621
[slava@slavadell performance]$

# HostArray iterator fancy object
copy_host_cached_to_host_FixDataHostCached                  {1024}                   IMPRECISE                  83
copy_host_cached_to_host_FixDataHostCached                  {524288}                 ok                      76302
copy_host_cached_to_host_FixDataHostCached                  {1048576}                ok                     282496
copy_host_cached_to_host_FixDataHostCached                  {536870912}              ok                  367287690
copy_host_to_host_cached_FixDataHostCached                  {1024}                   IMPRECISE                  82
copy_host_to_host_cached_FixDataHostCached                  {524288}                 ok                      84095
copy_host_to_host_cached_FixDataHostCached                  {1048576}                ok                     333638
copy_host_to_host_cached_FixDataHostCached                  {536870912}              ok                  385959375
copy_host_to_host_visible_FixDataHostVisible                {1024}                   IMPRECISE                  91
copy_host_to_host_visible_FixDataHostVisible                {524288}                 ok                     115304
copy_host_to_host_visible_FixDataHostVisible                {1048576}                ok                     233504
copy_host_to_host_visible_FixDataHostVisible                {536870912}              ok                  265843008
copy_host_visible_to_host_FixDataHostVisible                {1024}                   ok                      17678
copy_host_visible_to_host_FixDataHostVisible                {524288}                 ok                    9227576
copy_host_visible_to_host_FixDataHostVisible                {1048576}                ok                   18549500
copy_host_visible_to_host_FixDataHostVisible                {536870912}              ok                 9658979947
