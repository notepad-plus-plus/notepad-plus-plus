classdef Foo < handle

    % A couple of properties blocks
    properties (SetAccess = private)
        Var1
        Var2
    end

    properties
        Var3
        Var4
    end

    methods (Static)
        function y = f1(x)
            % events, properties and methods are the valid idenifiers
            % in the function scope
            events = 1;
            properties = 2;
            y = x + events * properties;
        end

        % Any of these words are also valid functions' names inside
        % methods block
        function y = events(x)
            
            arguments
                x {mustBeNegative}
            end

            y = f2(x)*100;
            function b = f2(a)
                b = a + 5;
            end
        end
    end

    % Example events block
    events
        Event1
        Event2
    end
end


% Now, let's break some stuff
classdef Bar

    properties
        % Though MATLAB won't execute such a code, events, properties
        % and methods are keywords here, because we're still in the class scope
        events
        end

        methods
        end        
    end
    
    % Not allowed in MATLAB, but, technically, we're still in the class scope
    if condition1
        if condition2
            % Though we're in the class scope, lexel will recognize no
            % keywords here: to avoid the neccessaty to track nested scopes,
            % it just considers everything beyond level 2 of folding to be
            % a function scope
            methods
            events
            properties
        end
    end


end

