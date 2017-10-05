function [mem] = getmemused()

    if(~isempty(strfind(lower(computer),'win')))
        
        userview = memory;
        mem = userview.MemUsedMATLAB;
        
    else

        [~,out]=system('vmstat -s -S M | grep "free memory"');
    
        mem=sscanf(out,'%f  free memory');
        mem = [num2str(mem) ' MB free memory'];
            
    
    end

end