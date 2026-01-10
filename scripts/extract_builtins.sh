#!/bin/bash

OUTPUT_FILE="$1"
shift
CPP_FILES="$@"

echo "{" > "$OUTPUT_FILE"

FIRST=1
for cpp_file in $CPP_FILES; do
    if [ -f "$cpp_file" ]; then
        [ $FIRST -eq 0 ] && echo "," >> "$OUTPUT_FILE"
        FIRST=0
        
        CATEGORY=$(basename "$cpp_file" .cpp | sed 's/_functions//')
        echo "  \"$CATEGORY\": [" >> "$OUTPUT_FILE"
        
        FUNC_FIRST=1
        DESC=""
        PARENT=""
        IN_CLASS=0
        
        while IFS= read -r line; do
            # Check for MATH_UNARY macro
            if [[ $line =~ MATH_UNARY\(([^,]+), ]]; then
                NAME="${BASH_REMATCH[1]}"
                [ $FUNC_FIRST -eq 0 ] && echo "," >> "$OUTPUT_FILE"
                FUNC_FIRST=0
                echo -n "    {\"name\": \"$NAME\", \"signature\": \"$NAME()\", \"description\": \"Mathematical function\", \"parent\": \"\"}" >> "$OUTPUT_FILE"
                continue
            fi
            
            # Check if entering a class
            if [[ $line =~ class.*BuiltinFunction ]]; then
                IN_CLASS=1
                DESC=""
                PARENT=""
            fi
            
            # Only process if we're in a class
            if [ $IN_CLASS -eq 1 ]; then
                # Extract @desc (can be on multiple lines)
                if [[ $line =~ //@desc[[:space:]]+(.*) ]]; then
                    if [ -n "$DESC" ]; then
                        DESC="$DESC ${BASH_REMATCH[1]}"
                    else
                        DESC="${BASH_REMATCH[1]}"
                    fi
                # Extract @parent
                elif [[ $line =~ //@parent[[:space:]]+(.*) ]]; then
                    PARENT="${BASH_REMATCH[1]}"
                # Extract function name
                elif [[ $line =~ getName.*return[[:space:]]+\"([^\"]+)\" ]]; then
                    NAME="${BASH_REMATCH[1]}"
                    [ $FUNC_FIRST -eq 0 ] && echo "," >> "$OUTPUT_FILE"
                    FUNC_FIRST=0
                    
                    SIG="$NAME()"
                    if [ -n "$PARENT" ]; then
                        SIG="$PARENT.$NAME()"
                    fi
                    
                    echo -n "    {\"name\": \"$NAME\", \"signature\": \"$SIG\", \"description\": \"$DESC\", \"parent\": \"$PARENT\"}" >> "$OUTPUT_FILE"
                    
                    # Reset for next function
                    DESC=""
                    PARENT=""
                    IN_CLASS=0
                fi
            fi
        done < "$cpp_file"
        
        echo "" >> "$OUTPUT_FILE"
        echo "  ]" >> "$OUTPUT_FILE"
    fi
done

echo "}" >> "$OUTPUT_FILE"

echo "Extracted builtins to $OUTPUT_FILE"
