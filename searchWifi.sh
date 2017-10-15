iwinfo wlan0 scan > /tmp/wifiscan #save scan results to a temp file
if [ -f /tmp/ssids ]; then
    rm /tmp/ssids
fi
n_results=$(grep -c "ESSID:" /tmp/wifiscan) #save number of scanned cell
i=1
while [ "$i" -le "$n_results" ]; do
    if [ $i -lt 10 ]; then
        cell=$(echo "Cell 0$i - Address:")
    else
        cell=$(echo "Cell $i - Address:")
    fi
    j=`expr $i + 1`
    if [ $j -lt 10 ]; then
        nextcell=$(echo "Cell 0$j - Address:")
    else
        nextcell=$(echo "Cell $j - Address:")
    fi
    awk -v v1="$cell" '$0 ~ v1 {p=1}p' /tmp/wifiscan | awk -v v2="$nextcell" '$0 ~ v2 {exit}1' > /tmp/onecell #store only one cell info in a temp file

    ##################################################
    ## Uncomment following line to show mac address ##

    #oneaddress=$(grep " Address:" /tmp/onecell | awk '{print $5}')

    onessid=$(grep "ESSID:" /tmp/onecell | awk '{ sub(/^[ \t]+/, ""); print }' | awk '{gsub("ESSID:", "");print}')
    if [ -n "$oneaddress" ]; then                                                                                                            
        echo "$onessid  $oneaddress $oneencryption $onepower" >> /tmp/ssids                                                              
    else                                                                                                                                     
        echo "$onessid  $oneencryption $onepower" >> /tmp/ssids                                                                          
    fi
    i=`expr $i + 1`
done
rm /tmp/onecell
awk '{printf("%s\n", $0)}' /tmp/ssids > /tmp/sec_ssids #add numbers at beginning of line
grep ESSID /tmp/wifiscan | awk '{ sub(/^[ \t]+/, ""); print }' | awk '{printf("%5d : %s\n", NR,$0)}' | awk '{gsub("ESSID:", "");print}' > /tmp/ssids #generate file with only numbers and names
cat /tmp/sec_ssids #show ssids list
