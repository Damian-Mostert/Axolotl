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
        RETURN_TYPE="void"
        PARAMS=""
        
        while IFS= read -r line; do
            # Extract description
            if [[ $line =~ @desc[[:space:]]+(.*) ]]; then
                DESC="${BASH_REMATCH[1]}"
            # Extract return type hint
            elif [[ $line =~ @return[[:space:]]+(.*) ]]; then
                RETURN_TYPE="${BASH_REMATCH[1]}"
            # Extract parameter hint
            elif [[ $line =~ @params[[:space:]]+(.*) ]]; then
                PARAMS="${BASH_REMATCH[1]}"
            # Extract function name
            elif [[ $line =~ getName.*return[[:space:]]+\"([^\"]+)\" ]]; then
                NAME="${BASH_REMATCH[1]}"
                [ $FUNC_FIRST -eq 0 ] && echo "," >> "$OUTPUT_FILE"
                FUNC_FIRST=0
                
                # Build signature
                if [ -n "$PARAMS" ]; then
                    SIG="$NAME($PARAMS)"
                else
                    SIG="$NAME()"
                fi
                
                if [ "$RETURN_TYPE" != "void" ]; then
                    SIG="$SIG -> $RETURN_TYPE"
                fi
                
                echo -n "    {\"name\": \"$NAME\", \"signature\": \"$SIG\", \"description\": \"$DESC\", \"returnType\": \"$RETURN_TYPE\"}" >> "$OUTPUT_FILE"
                
                # Reset for next function
                DESC=""
                RETURN_TYPE="void"
                PARAMS=""
            fi
        done < "$cpp_file"
        
        echo "" >> "$OUTPUT_FILE"
        echo "  ]" >> "$OUTPUT_FILE"
    fi
done

echo "}" >> "$OUTPUT_FILE"

echo "Extracted builtins to $OUTPUT_FILE"

# Extract parent types by parsing getParent() methods
PARENT_FILE="${OUTPUT_FILE%.json}_parents.json"
echo "{" > "$PARENT_FILE"
FIRST_PARENT=1
for cpp_file in $CPP_FILES; do
    if [ -f "$cpp_file" ]; then
        FUNC_NAME=""
        while IFS= read -r line; do
            if [[ $line =~ getName.*return[[:space:]]+\"([^\"]+)\" ]]; then
                FUNC_NAME="${BASH_REMATCH[1]}"
            elif [[ $line =~ getParent.*return[[:space:]]+\"([^\"]+)\" ]]; then
                PARENT_VAL="${BASH_REMATCH[1]}"
                if [ -n "$PARENT_VAL" ] && [ -n "$FUNC_NAME" ]; then
                    [ $FIRST_PARENT -eq 0 ] && echo "," >> "$PARENT_FILE"
                    FIRST_PARENT=0
                    echo -n "  \"$FUNC_NAME\": \"$PARENT_VAL\"" >> "$PARENT_FILE"
                fi
                FUNC_NAME=""
            fi
        done < "$cpp_file"
    fi
done
echo "" >> "$PARENT_FILE"
echo "}" >> "$PARENT_FILE"

echo "Extracted parent types to $PARENT_FILE"
