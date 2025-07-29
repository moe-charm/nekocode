export function getCompilerOption() {
                const compilerOptionName = moduleKind >= ModuleKind.ES2015
                    ? "allowSyntheticDefaultImports"
                    : "esModuleInterop";

                error(referencingLocation, Diagnostics.This_module_can_only_be_referenced_with_ECMAScript_imports_Slashexports_by_turning_on_the_0_flag_and_referencing_its_default_export, compilerOptionName);

                return symbol;
            }

            const referenceParent = referencingLocation.parent;
            const namespaceImport = isImportDeclaration(referenceParent) && getNamespaceDeclarationNode(referenceParent);
            if (namespaceImport || isImportCall(referenceParent)) {
                const reference = isImportCall(referenceParent) ? referenceParent.arguments[0] : referenceParent.moduleSpecifier;
                const type = getTypeOfSymbol(symbol);
                const defaultOnlyType = getTypeWithSyntheticDefaultOnly(type, symbol, moduleSymbol!, reference);
                if (defaultOnlyType) {
                    return cloneTypeAsModuleType(symbol, defaultOnlyType, referenceParent);
                }

                const targetFile = moduleSymbol?.declarations?.find(isSourceFile);
                const usageMode = getEmitSyntaxForModuleSpecifierExpression(reference);
                let exportModuleDotExportsSymbol: Symbol | undefined;
                if (
                    namespaceImport && targetFile &&
                    ModuleKind.Node20 <= moduleKind && moduleKind <= ModuleKind.NodeNext &&
                    usageMode === ModuleKind.CommonJS && host.getImpliedNodeFormatForEmit(targetFile) === ModuleKind.ESNext &&
                    (exportModuleDotExportsSymbol = resolveExportByName(symbol, "module.exports" as __String, namespaceImport, dontResolveAlias))
                ) {
                    if (!suppressInteropError && !(symbol.flags & (SymbolFlags.Module | SymbolFlags.Variable))) {
                        error(referencingLocation, Diagnostics.This_module_can_only_be_referenced_with_ECMAScript_imports_Slashexports_by_turning_on_the_0_flag_and_referencing_its_default_export, "esModuleInterop");
                    }
                    if (getESModuleInterop(compilerOptions) && hasSignatures(type)) {
                        return cloneTypeAsModuleType(exportModuleDotExportsSymbol, type, referenceParent);
                    }
                    return exportModuleDotExportsSymbol;
                }

                const isEsmCjsRef = targetFile && isESMFormatImportImportingCommonjsFormatFile(usageMode, host.getImpliedNodeFormatForEmit(targetFile));
                if (getESModuleInterop(compilerOptions) || isEsmCjsRef) {
                    if (
                        hasSignatures(type) ||
                        getPropertyOfType(type, InternalSymbolName.Default, /*skipObjectFunctionPropertyAugment*/ true) ||
                        isEsmCjsRef
                    ) {
                        const moduleType = type.flags & TypeFlags.StructuredType
                            ? getTypeWithSyntheticDefaultImportType(type, symbol, moduleSymbol!, reference)
                            : createDefaultPropertyWrapperForModule(symbol, symbol.parent);
                        return cloneTypeAsModuleType(symbol, moduleType, referenceParent);
                    }
                }
            }
        }
        return symbol;
    }

    function hasSignatures(type: Type): boolean {
        return some(getSignaturesOfStructuredType(type, SignatureKind.Call)) || some(getSignaturesOfStructuredType(type, SignatureKind.Construct));
    }

    /**
     * Create a new symbol which has the module's type less the call and construct signatures
     */
    function cloneTypeAsModuleType(symbol: Symbol, moduleType: Type, referenceParent: ImportDeclaration | ImportCall) {
        const result = createSymbol(symbol.flags, symbol.escapedName);
        result.declarations = symbol.declarations ? symbol.declarations.slice() : [];
        result.parent = symbol.parent;
        result.links.target = symbol;
        result.links.originatingImport = referenceParent;
        if (symbol.valueDeclaration) result.valueDeclaration = symbol.valueDeclaration;
        if (symbol.constEnumOnlyModule) result.constEnumOnlyModule = true;
        if (symbol.members) result.members = new Map(symbol.members);
        if (symbol.exports) result.exports = new Map(symbol.exports);
        const resolvedModuleType = resolveStructuredTypeMembers(moduleType as StructuredType); // Should already be resolved from the signature checks above
        result.links.type = createAnonymousType(result, resolvedModuleType.members, emptyArray, emptyArray, resolvedModuleType.indexInfos);
        return result;
    }

    function hasExportAssignmentSymbol(moduleSymbol: Symbol): boolean {
        return moduleSymbol.exports!.get(InternalSymbolName.ExportEquals) !== undefined;
    }

    function getExportsOfModuleAsArray(moduleSymbol: Symbol): Symbol[] {
        return symbolsToArray(getExportsOfModule(moduleSymbol));
    }

    function getExportsAndPropertiesOfModule(moduleSymbol: Symbol): Symbol[] {
        const exports = getExportsOfModuleAsArray(moduleSymbol);
        const exportEquals = resolveExternalModuleSymbol(moduleSymbol);
        if (exportEquals !== moduleSymbol) {
            const type = getTypeOfSymbol(exportEquals);
            if (shouldTreatPropertiesOfExternalModuleAsExports(type)) {
                addRange(exports, getPropertiesOfType(type));
            }
        }
        return exports;
    }

    function forEachExportAndPropertyOfModule(moduleSymbol: Symbol, cb: (symbol: Symbol, key: __String) => void): void {
        const exports = getExportsOfModule(moduleSymbol);
        exports.forEach((symbol, key) => {
            if (!isReservedMemberName(key)) {
                cb(symbol, key);
            }
        });
        const exportEquals = resolveExternalModuleSymbol(moduleSymbol);
        if (exportEquals !== moduleSymbol) {
            const type = getTypeOfSymbol(exportEquals);
            if (shouldTreatPropertiesOfExternalModuleAsExports(type)) {
                forEachPropertyOfType(type, (symbol, escapedName) => {
                    cb(symbol, escapedName);
                });
            }
        }
    }

    function tryGetMemberInModuleExports(memberName: __String, moduleSymbol: Symbol): Symbol | undefined {
        const symbolTable = getExportsOfModule(moduleSymbol);
        if (symbolTable) {
            return symbolTable.get(memberName);
        }
    }

    function tryGetMemberInModuleExportsAndProperties(memberName: __String, moduleSymbol: Symbol): Symbol | undefined {
        const symbol = tryGetMemberInModuleExports(memberName, moduleSymbol);
        if (symbol) {
            return symbol;
        }

        const exportEquals = resolveExternalModuleSymbol(moduleSymbol);
        if (exportEquals === moduleSymbol) {
            return undefined;
        }

        const type = getTypeOfSymbol(exportEquals);
        return shouldTreatPropertiesOfExternalModuleAsExports(type) ? getPropertyOfType(type, memberName) : undefined;
    }

    function shouldTreatPropertiesOfExternalModuleAsExports(resolvedExternalModuleType: Type) {
        return !(resolvedExternalModuleType.flags & TypeFlags.Primitive ||
            getObjectFlags(resolvedExternalModuleType) & ObjectFlags.Class ||
            // `isArrayOrTupleLikeType` is too expensive to use in this auto-imports hot path
            isArrayType(resolvedExternalModuleType) ||
            isTupleType(resolvedExternalModuleType));
    }

    function getExportsOfSymbol(symbol: Symbol): SymbolTable {
        return symbol.flags & SymbolFlags.LateBindingContainer ? getResolvedMembersOrExportsOfSymbol(symbol, MembersOrExportsResolutionKind.resolvedExports) :
            symbol.flags & SymbolFlags.Module ? getExportsOfModule(symbol) :
            symbol.exports || emptySymbols;
    }

    function getExportsOfModule(moduleSymbol: Symbol): SymbolTable {
        const links = getSymbolLinks(moduleSymbol);
        if (!links.resolvedExports) {
            const { exports, typeOnlyExportStarMap } = getExportsOfModuleWorker(moduleSymbol);
            links.resolvedExports = exports;
            links.typeOnlyExportStarMap = typeOnlyExportStarMap;
        }
        return links.resolvedExports;
    }

    interface ExportCollisionTracker {
        specifierText: string;
        exportsWithDuplicate?: ExportDeclaration[];
    }

    type ExportCollisionTrackerTable = Map<__String, ExportCollisionTracker>;

    /**
     * Extends one symbol table with another while collecting information on name collisions for error message generation into the `lookupTable` argument
     * Not passing `lookupTable` and `exportNode` disables this collection, and just extends the tables
     */
    function extendExportSymbols(target: SymbolTable, source: SymbolTable | undefined, lookupTable?: ExportCollisionTrackerTable, exportNode?: ExportDeclaration) {
        if (!source) return;
        source.forEach((sourceSymbol, id) => {
            if (id === InternalSymbolName.Default) return;

            const targetSymbol = target.get(id);
            if (!targetSymbol) {
                target.set(id, sourceSymbol);
                if (lookupTable && exportNode) {
                    lookupTable.set(id, {
                        specifierText: getTextOfNode(exportNode.moduleSpecifier!),
                    });
                }
            }
            else if (lookupTable && exportNode && targetSymbol && resolveSymbol(targetSymbol) !== resolveSymbol(sourceSymbol)) {
                const collisionTracker = lookupTable.get(id)!;
                if (!collisionTracker.exportsWithDuplicate) {
                    collisionTracker.exportsWithDuplicate = [exportNode];
                }
                else {
                    collisionTracker.exportsWithDuplicate.push(exportNode);
                }
            }
        });
    }

    function getExportsOfModuleWorker(moduleSymbol: Symbol) {
        const visitedSymbols: Symbol[] = [];
        let typeOnlyExportStarMap: Map<__String, ExportDeclaration & { readonly isTypeOnly: true; readonly moduleSpecifier: Expression; }> | undefined;
        const nonTypeOnlyNames = new Set<__String>();

        // A module defined by an 'export=' consists of one export that needs to be resolved
        moduleSymbol = resolveExternalModuleSymbol(moduleSymbol);
        const exports = visit(moduleSymbol) || emptySymbols;

        if (typeOnlyExportStarMap) {
            nonTypeOnlyNames.forEach(name => typeOnlyExportStarMap!.delete(name));
        }

        return {
            exports,
            typeOnlyExportStarMap,
        };

        // The ES6 spec permits export * declarations in a module to circularly reference the module itself. For example,
        // module 'a' can 'export * from "b"' and 'b' can 'export * from "a"' without error.
        function visit(symbol: Symbol | undefined, exportStar?: ExportDeclaration, isTypeOnly?: boolean): SymbolTable | undefined {
            if (!isTypeOnly && symbol?.exports) {
                // Add non-type-only names before checking if we've visited this module,
                // because we might have visited it via an 'export type *', and visiting
                // again with 'export *' will override the type-onlyness of its exports.
                symbol.exports.forEach((_, name) => nonTypeOnlyNames.add(name));
            }
            if (!(symbol && symbol.exports && pushIfUnique(visitedSymbols, symbol))) {
                return;
            }
            const symbols = new Map(symbol.exports);

            // All export * declarations are collected in an __export symbol by the binder
            const exportStars = symbol.exports.get(InternalSymbolName.ExportStar);
            if (exportStars) {
                const nestedSymbols = createSymbolTable();
                const lookupTable: ExportCollisionTrackerTable = new Map();
                if (exportStars.declarations) {
                    for (const node of exportStars.declarations) {
                        const resolvedModule = resolveExternalModuleName(node, (node as ExportDeclaration).moduleSpecifier!);
                        const exportedSymbols = visit(resolvedModule, node as ExportDeclaration, isTypeOnly || (node as ExportDeclaration).isTypeOnly);
                        extendExportSymbols(
                            nestedSymbols,
                            exportedSymbols,
                            lookupTable,
                            node as ExportDeclaration,
                        );
                    }
                }
                lookupTable.forEach(({ exportsWithDuplicate }, id) => {
                    // It's not an error if the file with multiple `export *`s with duplicate names exports a member with that name itself
                    if (id === "export=" || !(exportsWithDuplicate && exportsWithDuplicate.length) || symbols.has(id)) {
                        return;
                    }
                    for (const node of exportsWithDuplicate) {
                        diagnostics.add(createDiagnosticForNode(
                            node,
                            Diagnostics.Module_0_has_already_exported_a_member_named_1_Consider_explicitly_re_exporting_to_resolve_the_ambiguity,
                            lookupTable.get(id)!.specifierText,
                            unescapeLeadingUnderscores(id),
                        ));
                    }
                });
                extendExportSymbols(symbols, nestedSymbols);
            }
            if (exportStar?.isTypeOnly) {
                typeOnlyExportStarMap ??= new Map();
                symbols.forEach((_, escapedName) =>
                    typeOnlyExportStarMap!.set(
                        escapedName,
                        exportStar as ExportDeclaration & { readonly isTypeOnly: true; readonly moduleSpecifier: Expression; },
                    )
                );
            }
            return symbols;
        }
    }

    function getMergedSymbol(symbol: Symbol): Symbol;
    function getMergedSymbol(symbol: Symbol | undefined): Symbol | undefined;
    function getMergedSymbol(symbol: Symbol | undefined): Symbol | undefined {
        let merged: Symbol;
        return symbol && symbol.mergeId && (merged = mergedSymbols[symbol.mergeId]) ? merged : symbol;
    }

    function getSymbolOfDeclaration(node: Declaration): Symbol {
        return getMergedSymbol(node.symbol && getLateBoundSymbol(node.symbol));
    }

    /**
     * Get the merged symbol for a node. If you know the node is a `Declaration`, it is faster and more type safe to
     * use use `getSymbolOfDeclaration` instead.
     */
    function getSymbolOfNode(node: Node): Symbol | undefined {
        return canHaveSymbol(node) ? getSymbolOfDeclaration(node) : undefined;
    }

    function getParentOfSymbol(symbol: Symbol): Symbol | undefined {
        return getMergedSymbol(symbol.parent && getLateBoundSymbol(symbol.parent));
    }

    function getFunctionExpressionParentSymbolOrSymbol(symbol: Symbol) {
        return symbol.valueDeclaration?.kind === SyntaxKind.ArrowFunction || symbol.valueDeclaration?.kind === SyntaxKind.FunctionExpression
            ? getSymbolOfNode(symbol.valueDeclaration.parent) || symbol
            : symbol;
    }

    function getAlternativeContainingModules(symbol: Symbol, enclosingDeclaration: Node): Symbol[] {
        const containingFile = getSourceFileOfNode(enclosingDeclaration);
        const id = getNodeId(containingFile);
        const links = getSymbolLinks(symbol);
        let results: Symbol[] | undefined;
        if (links.extendedContainersByFile && (results = links.extendedContainersByFile.get(id))) {
            return results;
        }
        if (containingFile && containingFile.imports) {
            // Try to make an import using an import already in the enclosing file, if possible
            for (const importRef of containingFile.imports) {
                if (nodeIsSynthesized(importRef)) continue; // Synthetic names can't be resolved by `resolveExternalModuleName` - they'll cause a debug assert if they error
                const resolvedModule = resolveExternalModuleName(enclosingDeclaration, importRef, /*ignoreErrors*/ true);
                if (!resolvedModule) continue;
                const ref = getAliasForSymbolInContainer(resolvedModule, symbol);
                if (!ref) continue;
                results = append(results, resolvedModule);
            }
            if (length(results)) {
                (links.extendedContainersByFile || (links.extendedContainersByFile = new Map())).set(id, results!);
                return results!;
            }
        }
        if (links.extendedContainers) {
            return links.extendedContainers;
        }
        // No results from files already being imported by this file - expand search (expensive, but not location-specific, so cached)
        const otherFiles = host.getSourceFiles();
        for (const file of otherFiles) {
            if (!isExternalModule(file)) continue;
            const sym = getSymbolOfDeclaration(file);
            const ref = getAliasForSymbolInContainer(sym, symbol);
            if (!ref) continue;
            results = append(results, sym);
        }
        return links.extendedContainers = results || emptyArray;
    }

    /**
     * Attempts to find the symbol corresponding to the container a symbol is in - usually this
     * is just its' `.parent`, but for locals, this value is `undefined`
     */
    function getContainersOfSymbol(symbol: Symbol, enclosingDeclaration: Node | undefined, meaning: SymbolFlags): Symbol[] | undefined {
        const container = getParentOfSymbol(symbol);
        // Type parameters end up in the `members` lists but are not externally visible
        if (container && !(symbol.flags & SymbolFlags.TypeParameter)) {
            return getWithAlternativeContainers(container);
        }
        const candidates = mapDefined(symbol.declarations, d => {
            if (!isAmbientModule(d) && d.parent) {
                // direct children of a module
                if (hasNonGlobalAugmentationExternalModuleSymbol(d.parent)) {
                    return getSymbolOfDeclaration(d.parent as Declaration);
                }
                // export ='d member of an ambient module
                if (isModuleBlock(d.parent) && d.parent.parent && resolveExternalModuleSymbol(getSymbolOfDeclaration(d.parent.parent)) === symbol) {
                    return getSymbolOfDeclaration(d.parent.parent);
                }
            }
            if (isClassExpression(d) && isBinaryExpression(d.parent) && d.parent.operatorToken.kind === SyntaxKind.EqualsToken && isAccessExpression(d.parent.left) && isEntityNameExpression(d.parent.left.expression)) {
                if (isModuleExportsAccessExpression(d.parent.left) || isExportsIdentifier(d.parent.left.expression)) {
                    return getSymbolOfDeclaration(getSourceFileOfNode(d));
                }
                checkExpressionCached(d.parent.left.expression);
                return getNodeLinks(d.parent.left.expression).resolvedSymbol;
            }
        });
        if (!length(candidates)) {
            return undefined;
        }
        const containers = mapDefined(candidates, candidate => getAliasForSymbolInContainer(candidate, symbol) ? candidate : undefined);

        let bestContainers: Symbol[] = [];
        let alternativeContainers: Symbol[] = [];

        for (const container of containers) {
            const [bestMatch, ...rest] = getWithAlternativeContainers(container);
            bestContainers = append(bestContainers, bestMatch);
            alternativeContainers = addRange(alternativeContainers, rest);
        }

        return concatenate(bestContainers, alternativeContainers);

        function getWithAlternativeContainers(container: Symbol) {
            const additionalContainers = mapDefined(container.declarations, fileSymbolIfFileSymbolExportEqualsContainer);
            const reexportContainers = enclosingDeclaration && getAlternativeContainingModules(symbol, enclosingDeclaration);
            const objectLiteralContainer = getVariableDeclarationOfObjectLiteral(container, meaning);
            if (
                enclosingDeclaration &&
                container.flags & getQualifiedLeftMeaning(meaning) &&
                getAccessibleSymbolChain(container, enclosingDeclaration, SymbolFlags.Namespace, /*useOnlyExternalAliasing*/ false)
            ) {
                return append(concatenate(concatenate([container], additionalContainers), reexportContainers), objectLiteralContainer); // This order expresses a preference for the real container if it is in scope
            }
            // we potentially have a symbol which is a member of the instance side of something - look for a variable in scope with the container's type
            // which may be acting like a namespace (eg, `Symbol` acts like a namespace when looking up `Symbol.toStringTag`)
            const firstVariableMatch = !(container.flags & getQualifiedLeftMeaning(meaning))
                    && container.flags & SymbolFlags.Type
                    && getDeclaredTypeOfSymbol(container).flags & TypeFlags.Object
                    && meaning === SymbolFlags.Value
                ? forEachSymbolTableInScope(enclosingDeclaration, t => {
                    return forEachEntry(t, s => {
                        if (s.flags & getQualifiedLeftMeaning(meaning) && getTypeOfSymbol(s) === getDeclaredTypeOfSymbol(container)) {
                            return s;
                        }
                    });
                }) : undefined;
            let res = firstVariableMatch ? [firstVariableMatch, ...additionalContainers, container] : [...additionalContainers, container];
            res = append(res, objectLiteralContainer);
            res = addRange(res, reexportContainers);
            return res;
        }

        function fileSymbolIfFileSymbolExportEqualsContainer(d: Declaration) {
            return container && getFileSymbolIfFileSymbolExportEqualsContainer(d, container);
        }
    }

    function getVariableDeclarationOfObjectLiteral(symbol: Symbol, meaning: SymbolFlags) {
        // If we're trying to reference some object literal in, eg `var a = { x: 1 }`, the symbol for the literal, `__object`, is distinct
        // from the symbol of the declaration it is being assigned to. Since we can use the declaration to refer to the literal, however,
        // we'd like to make that connection here - potentially causing us to paint the declaration's visibility, and therefore the literal.
        const firstDecl: Node | false = !!length(symbol.declarations) && first(symbol.declarations!);
        if (meaning & SymbolFlags.Value && firstDecl && firstDecl.parent && isVariableDeclaration(firstDecl.parent)) {
            if (isObjectLiteralExpression(firstDecl) && firstDecl === firstDecl.parent.initializer || isTypeLiteralNode(firstDecl) && firstDecl === firstDecl.parent.type) {
                return getSymbolOfDeclaration(firstDecl.parent);
            }
        }
    }

    function getFileSymbolIfFileSymbolExportEqualsContainer(d: Declaration, container: Symbol) {
        const fileSymbol = getExternalModuleContainer(d);
        const exported = fileSymbol && fileSymbol.exports && fileSymbol.exports.get(InternalSymbolName.ExportEquals);
        return exported && getSymbolIfSameReference(exported, container) ? fileSymbol : undefined;
    }

    function getAliasForSymbolInContainer(container: Symbol, symbol: Symbol) {
        if (container === getParentOfSymbol(symbol)) {
            // fast path, `symbol` is either already the alias or isn't aliased
            return symbol;
        }
        // Check if container is a thing with an `export=` which points directly at `symbol`, and if so, return
        // the container itself as the alias for the symbol
        const exportEquals = container.exports && container.exports.get(InternalSymbolName.ExportEquals);
        if (exportEquals && getSymbolIfSameReference(exportEquals, symbol)) {
            return container;
        }
        const exports = getExportsOfSymbol(container);
        const quick = exports.get(symbol.escapedName);
        if (quick && getSymbolIfSameReference(quick, symbol)) {
            return quick;
        }
        return forEachEntry(exports, exported => {
            if (getSymbolIfSameReference(exported, symbol)) {
                return exported;
            }
        });
    }

    /**
     * Checks if two symbols, through aliasing and/or merging, refer to the same thing
     */
    function getSymbolIfSameReference(s1: Symbol, s2: Symbol) {
        if (getMergedSymbol(resolveSymbol(getMergedSymbol(s1))) === getMergedSymbol(resolveSymbol(getMergedSymbol(s2)))) {
            return s1;
        }
    }

    function getExportSymbolOfValueSymbolIfExported(symbol: Symbol): Symbol;
    function getExportSymbolOfValueSymbolIfExported(symbol: Symbol | undefined): Symbol | undefined;
    function getExportSymbolOfValueSymbolIfExported(symbol: Symbol | undefined): Symbol | undefined {
        return getMergedSymbol(symbol && (symbol.flags & SymbolFlags.ExportValue) !== 0 && symbol.exportSymbol || symbol);
    }

    function symbolIsValue(symbol: Symbol, includeTypeOnlyMembers?: boolean): boolean {
        return !!(
            symbol.flags & SymbolFlags.Value ||
            symbol.flags & SymbolFlags.Alias && getSymbolFlags(symbol, !includeTypeOnlyMembers) & SymbolFlags.Value
        );
    }

    function createType(flags: TypeFlags): Type {
        const result = new Type(checker, flags);
        typeCount++;
        result.id = typeCount;
        tracing?.recordType(result);
        return result;
    }

    function createTypeWithSymbol(flags: TypeFlags, symbol: Symbol): Type {
        const result = createType(flags);
        result.symbol = symbol;
        return result;
    }

    function createOriginType(flags: TypeFlags): Type {
        return new Type(checker, flags);
    }

    function createIntrinsicType(kind: TypeFlags, intrinsicName: string, objectFlags = ObjectFlags.None, debugIntrinsicName?: string): IntrinsicType {
        checkIntrinsicName(intrinsicName, debugIntrinsicName);
        const type = createType(kind) as IntrinsicType;
        type.intrinsicName = intrinsicName;
        type.debugIntrinsicName = debugIntrinsicName;
        type.objectFlags = objectFlags | ObjectFlags.CouldContainTypeVariablesComputed | ObjectFlags.IsGenericTypeComputed | ObjectFlags.IsUnknownLikeUnionComputed | ObjectFlags.IsNeverIntersectionComputed;
        return type;
    }

    function checkIntrinsicName(name: string, debug: string | undefined) {
        const key = `${name},${debug ?? ""}`;
        if (seenIntrinsicNames.has(key)) {
            Debug.fail(`Duplicate intrinsic type name ${name}${debug ? ` (${debug})` : ""}; you may need to pass a name to createIntrinsicType.`);
        }
        seenIntrinsicNames.add(key);
    }

    function createObjectType(objectFlags: ObjectFlags, symbol?: Symbol): ObjectType {
        const type = createTypeWithSymbol(TypeFlags.Object, symbol!) as ObjectType;
        type.objectFlags = objectFlags;
        type.members = undefined;
        type.properties = undefined;
        type.callSignatures = undefined;
        type.constructSignatures = undefined;
        type.indexInfos = undefined;
        return type;
    }

    function createTypeofType() {
        return getUnionType(arrayFrom(typeofNEFacts.keys(), getStringLiteralType));
    }

    function createTypeParameter(symbol?: Symbol) {
        return createTypeWithSymbol(TypeFlags.TypeParameter, symbol!) as TypeParameter;
    }

    // A reserved member name starts with two underscores, but the third character cannot be an underscore,
    // @, or #. A third underscore indicates an escaped form of an identifier that started
    // with at least two underscores. The @ character indicates that the name is denoted by a well known ES
    // Symbol instance and the # character indicates that the name is a PrivateIdentifier.
    function isReservedMemberName(name: __String) {
        return (name as string).charCodeAt(0) === CharacterCodes._ &&
            (name as string).charCodeAt(1) === CharacterCodes._ &&
            (name as string).charCodeAt(2) !== CharacterCodes._ &&
            (name as string).charCodeAt(2) !== CharacterCodes.at &&
            (name as string).charCodeAt(2) !== CharacterCodes.hash;
    }

    function getNamedMembers(members: SymbolTable): Symbol[] {
        let result: Symbol[] | undefined;
        members.forEach((symbol, id) => {
            if (isNamedMember(symbol, id)) {
                (result || (result = [])).push(symbol);
            }
        });
        return result || emptyArray;
    }

    function isNamedMember(member: Symbol, escapedName: __String) {
        return !isReservedMemberName(escapedName) && symbolIsValue(member);
    }

    function getNamedOrIndexSignatureMembers(members: SymbolTable): Symbol[] {
        const result = getNamedMembers(members);
        const index = getIndexSymbolFromSymbolTable(members);
        return index ? concatenate(result, [index]) : result;
    }

    function setStructuredTypeMembers(type: StructuredType, members: SymbolTable, callSignatures: readonly Signature[], constructSignatures: readonly Signature[], indexInfos: readonly IndexInfo[]): ResolvedType {
        const resolved = type as ResolvedType;
        resolved.members = members;
        resolved.properties = emptyArray;
        resolved.callSignatures = callSignatures;
        resolved.constructSignatures = constructSignatures;
        resolved.indexInfos = indexInfos;
        // This can loop back to getPropertyOfType() which would crash if `callSignatures` & `constructSignatures` are not initialized.
        if (members !== emptySymbols) resolved.properties = getNamedMembers(members);
        return resolved;
    }

    function createAnonymousType(symbol: Symbol | undefined, members: SymbolTable, callSignatures: readonly Signature[], constructSignatures: readonly Signature[], indexInfos: readonly IndexInfo[]): ResolvedType {
        return setStructuredTypeMembers(createObjectType(ObjectFlags.Anonymous, symbol), members, callSignatures, constructSignatures, indexInfos);
    }

    function getResolvedTypeWithoutAbstractConstructSignatures(type: ResolvedType) {
        if (type.constructSignatures.length === 0) return type;
        if (type.objectTypeWithoutAbstractConstructSignatures) return type.objectTypeWithoutAbstractConstructSignatures;
        const constructSignatures = filter(type.constructSignatures, signature => !(signature.flags & SignatureFlags.Abstract));
        if (type.constructSignatures === constructSignatures) return type;
        const typeCopy = createAnonymousType(
            type.symbol,
            type.members,
            type.callSignatures,
            some(constructSignatures) ? constructSignatures : emptyArray,
            type.indexInfos,
        );
        type.objectTypeWithoutAbstractConstructSignatures = typeCopy;
        typeCopy.objectTypeWithoutAbstractConstructSignatures = typeCopy;
        return typeCopy;
    }

    function forEachSymbolTableInScope<T>(enclosingDeclaration: Node | undefined, callback: (symbolTable: SymbolTable, ignoreQualification?: boolean, isLocalNameLookup?: boolean, scopeNode?: Node) => T): T {
        let result: T;
        for (let location = enclosingDeclaration; location; location = location.parent) {
            // Locals of a source file are not in scope (because they get merged into the global symbol table)
            if (canHaveLocals(location) && location.locals && !isGlobalSourceFile(location)) {
                if (result = callback(location.locals, /*ignoreQualification*/ undefined, /*isLocalNameLookup*/ true, location)) {
                    return result;
                }
            }
            switch (location.kind) {
                case SyntaxKind.SourceFile:
                    if (!isExternalOrCommonJsModule(location as SourceFile)) {
                        break;
                    }
                    // falls through
                case SyntaxKind.ModuleDeclaration:
                    const sym = getSymbolOfDeclaration(location as ModuleDeclaration);
                    // `sym` may not have exports if this module declaration is backed by the symbol for a `const` that's being rewritten
                    // into a namespace - in such cases, it's best to just let the namespace appear empty (the const members couldn't have referred
                    // to one another anyway)
                    if (result = callback(sym?.exports || emptySymbols, /*ignoreQualification*/ undefined, /*isLocalNameLookup*/ true, location)) {
                        return result;
                    }
                    break;
                case SyntaxKind.ClassDeclaration:
                case SyntaxKind.ClassExpression:
                case SyntaxKind.InterfaceDeclaration:
                    // Type parameters are bound into `members` lists so they can merge across declarations
                    // This is troublesome, since in all other respects, they behave like locals :cries:
                    // TODO: the below is shared with similar code in `resolveName` - in fact, rephrasing all this symbol
                    // lookup logic in terms of `resolveName` would be nice
                    // The below is used to lookup type parameters within a class or interface, as they are added to the class/interface locals
                    // These can never be latebound, so the symbol's raw members are sufficient. `getMembersOfNode` cannot be used, as it would
                    // trigger resolving late-bound names, which we may already be in the process of doing while we're here!
                    let table: Map<__String, Symbol> | undefined;
                    // TODO: Should this filtered table be cached in some way?
                    (getSymbolOfDeclaration(location as ClassLikeDeclaration | InterfaceDeclaration).members || emptySymbols).forEach((memberSymbol, key) => {
                        if (memberSymbol.flags & (SymbolFlags.Type & ~SymbolFlags.Assignment)) {
                            (table || (table = createSymbolTable())).set(key, memberSymbol);
                        }
                    });
                    if (table && (result = callback(table, /*ignoreQualification*/ undefined, /*isLocalNameLookup*/ false, location))) {
                        return result;
                    }
                    break;
            }
        }

        return callback(globals, /*ignoreQualification*/ undefined, /*isLocalNameLookup*/ true);
    }

    function getQualifiedLeftMeaning(rightMeaning: SymbolFlags) {
        // If we are looking in value space, the parent meaning is value, other wise it is namespace
        return rightMeaning === SymbolFlags.Value ? SymbolFlags.Value : SymbolFlags.Namespace;
    }

    function getAccessibleSymbolChain(symbol: Symbol | undefined, enclosingDeclaration: Node | undefined, meaning: SymbolFlags, useOnlyExternalAliasing: boolean, visitedSymbolTablesMap = new Map<SymbolId, SymbolTable[]>()): Symbol[] | undefined {
        if (!(symbol && !isPropertyOrMethodDeclarationSymbol(symbol))) {
            return undefined;
        }
        const links = getSymbolLinks(symbol);
        const cache = (links.accessibleChainCache ||= new Map());
        // Go from enclosingDeclaration to the first scope we check, so the cache is keyed off the scope and thus shared more
        const firstRelevantLocation = forEachSymbolTableInScope(enclosingDeclaration, (_, __, ___, node) => node);
        const key = `${useOnlyExternalAliasing ? 0 : 1}|${firstRelevantLocation ? getNodeId(firstRelevantLocation) : 0}|${meaning}`;
        if (cache.has(key)) {
            return cache.get(key);
        }

        const id = getSymbolId(symbol);
        let visitedSymbolTables = visitedSymbolTablesMap.get(id);
        if (!visitedSymbolTables) {
            visitedSymbolTablesMap.set(id, visitedSymbolTables = []);
        }
        const result = forEachSymbolTableInScope(enclosingDeclaration, getAccessibleSymbolChainFromSymbolTable);
        cache.set(key, result);
        return result;

        /**
         * @param {ignoreQualification} boolean Set when a symbol is being looked for through the exports of another symbol (meaning we have a route to qualify it already)
         */
        function getAccessibleSymbolChainFromSymbolTable(symbols: SymbolTable, ignoreQualification?: boolean, isLocalNameLookup?: boolean): Symbol[] | undefined {
            if (!pushIfUnique(visitedSymbolTables!, symbols)) {
                return undefined;
            }

            const result = trySymbolTable(symbols, ignoreQualification, isLocalNameLookup);
            visitedSymbolTables!.pop();
            return result;
        }

        function canQualifySymbol(symbolFromSymbolTable: Symbol, meaning: SymbolFlags) {
            // If the symbol is equivalent and doesn't need further qualification, this symbol is accessible
            return !needsQualification(symbolFromSymbolTable, enclosingDeclaration, meaning) ||
                // If symbol needs qualification, make sure that parent is accessible, if it is then this symbol is accessible too
                !!getAccessibleSymbolChain(symbolFromSymbolTable.parent, enclosingDeclaration, getQualifiedLeftMeaning(meaning), useOnlyExternalAliasing, visitedSymbolTablesMap);
        }

        function isAccessible(symbolFromSymbolTable: Symbol, resolvedAliasSymbol?: Symbol, ignoreQualification?: boolean) {
            return (symbol === (resolvedAliasSymbol || symbolFromSymbolTable) || getMergedSymbol(symbol) === getMergedSymbol(resolvedAliasSymbol || symbolFromSymbolTable)) &&
                // if the symbolFromSymbolTable is not external module (it could be if it was determined as ambient external module and would be in globals table)
                // and if symbolFromSymbolTable or alias resolution matches the symbol,
                // check the symbol can be qualified, it is only then this symbol is accessible
                !some(symbolFromSymbolTable.declarations, hasNonGlobalAugmentationExternalModuleSymbol) &&
                (ignoreQualification || canQualifySymbol(getMergedSymbol(symbolFromSymbolTable), meaning));
        }

        function trySymbolTable(symbols: SymbolTable, ignoreQualification: boolean | undefined, isLocalNameLookup: boolean | undefined): Symbol[] | undefined {
            // If symbol is directly available by its name in the symbol table
            if (isAccessible(symbols.get(symbol!.escapedName)!, /*resolvedAliasSymbol*/ undefined, ignoreQualification)) {
                return [symbol!];
            }

            // Check if symbol is any of the aliases in scope
            const result = forEachEntry(symbols, symbolFromSymbolTable => {
                if (
                    symbolFromSymbolTable.flags & SymbolFlags.Alias
                    && symbolFromSymbolTable.escapedName !== InternalSymbolName.ExportEquals
                    && symbolFromSymbolTable.escapedName !== InternalSymbolName.Default
                    && !(isUMDExportSymbol(symbolFromSymbolTable) && enclosingDeclaration && isExternalModule(getSourceFileOfNode(enclosingDeclaration)))
                    // If `!useOnlyExternalAliasing`, we can use any type of alias to get the name
                    && (!useOnlyExternalAliasing || some(symbolFromSymbolTable.declarations, isExternalModuleImportEqualsDeclaration))
                    // If we're looking up a local name to reference directly, omit namespace reexports, otherwise when we're trawling through an export list to make a dotted name, we can keep it
                    && (isLocalNameLookup ? !some(symbolFromSymbolTable.declarations, isNamespaceReexportDeclaration) : true)
                    // While exports are generally considered to be in scope, export-specifier declared symbols are _not_
                    // See similar comment in `resolveName` for details
                    && (ignoreQualification || !getDeclarationOfKind(symbolFromSymbolTable, SyntaxKind.ExportSpecifier))
                ) {
                    const resolvedImportedSymbol = resolveAlias(symbolFromSymbolTable);
                    const candidate = getCandidateListForSymbol(symbolFromSymbolTable, resolvedImportedSymbol, ignoreQualification);
                    if (candidate) {
                        return candidate;
                    }
                }
                if (symbolFromSymbolTable.escapedName === symbol!.escapedName && symbolFromSymbolTable.exportSymbol) {
                    if (isAccessible(getMergedSymbol(symbolFromSymbolTable.exportSymbol), /*resolvedAliasSymbol*/ undefined, ignoreQualification)) {
                        return [symbol!];
                    }
                }
            });

            // If there's no result and we're looking at the global symbol table, treat `globalThis` like an alias and try to lookup thru that
            return result || (symbols === globals ? getCandidateListForSymbol(globalThisSymbol, globalThisSymbol, ignoreQualification) : undefined);
        }

        function getCandidateListForSymbol(symbolFromSymbolTable: Symbol, resolvedImportedSymbol: Symbol, ignoreQualification: boolean | undefined) {
            if (isAccessible(symbolFromSymbolTable, resolvedImportedSymbol, ignoreQualification)) {
                return [symbolFromSymbolTable];
            }

            // Look in the exported members, if we can find accessibleSymbolChain, symbol is accessible using this chain
            // but only if the symbolFromSymbolTable can be qualified
            const candidateTable = getExportsOfSymbol(resolvedImportedSymbol);
            const accessibleSymbolsFromExports = candidateTable && getAccessibleSymbolChainFromSymbolTable(candidateTable, /*ignoreQualification*/ true);
            if (accessibleSymbolsFromExports && canQualifySymbol(symbolFromSymbolTable, getQualifiedLeftMeaning(meaning))) {
                return [symbolFromSymbolTable].concat(accessibleSymbolsFromExports);
            }
        }
    }

    function needsQualification(symbol: Symbol, enclosingDeclaration: Node | undefined, meaning: SymbolFlags) {
        let qualify = false;
        forEachSymbolTableInScope(enclosingDeclaration, symbolTable => {
            // If symbol of this name is not available in the symbol table we are ok
            let symbolFromSymbolTable = getMergedSymbol(symbolTable.get(symbol.escapedName));
            if (!symbolFromSymbolTable) {
                // Continue to the next symbol table
                return false;
            }
            // If the symbol with this name is present it should refer to the symbol
            if (symbolFromSymbolTable === symbol) {
                // No need to qualify
                return true;
            }

            // Qualify if the symbol from symbol table has same meaning as expected
            const shouldResolveAlias = symbolFromSymbolTable.flags & SymbolFlags.Alias && !getDeclarationOfKind(symbolFromSymbolTable, SyntaxKind.ExportSpecifier);
            symbolFromSymbolTable = shouldResolveAlias ? resolveAlias(symbolFromSymbolTable) : symbolFromSymbolTable;
            const flags = shouldResolveAlias ? getSymbolFlags(symbolFromSymbolTable) : symbolFromSymbolTable.flags;
            if (flags & meaning) {
                qualify = true;
                return true;
            }

            // Continue to the next symbol table
            return false;
        });

        return qualify;
    }

    function isPropertyOrMethodDeclarationSymbol(symbol: Symbol) {
        if (symbol.declarations && symbol.declarations.length) {
            for (const declaration of symbol.declarations) {
                switch (declaration.kind) {
                    case SyntaxKind.PropertyDeclaration:
                    case SyntaxKind.MethodDeclaration:
                    case SyntaxKind.GetAccessor:
                    case SyntaxKind.SetAccessor:
                        continue;
                    default:
                        return false;
                }
            }
            return true;
        }
        return false;
    }

    function isTypeSymbolAccessible(typeSymbol: Symbol, enclosingDeclaration: Node | undefined): boolean {
        const access = isSymbolAccessibleWorker(typeSymbol, enclosingDeclaration, SymbolFlags.Type, /*shouldComputeAliasesToMakeVisible*/ false, /*allowModules*/ true);
        return access.accessibility === SymbolAccessibility.Accessible;
    }

    function isValueSymbolAccessible(typeSymbol: Symbol, enclosingDeclaration: Node | undefined): boolean {
        const access = isSymbolAccessibleWorker(typeSymbol, enclosingDeclaration, SymbolFlags.Value, /*shouldComputeAliasesToMakeVisible*/ false, /*allowModules*/ true);
        return access.accessibility === SymbolAccessibility.Accessible;
    }

    function isSymbolAccessibleByFlags(typeSymbol: Symbol, enclosingDeclaration: Node | undefined, flags: SymbolFlags): boolean {
        const access = isSymbolAccessibleWorker(typeSymbol, enclosingDeclaration, flags, /*shouldComputeAliasesToMakeVisible*/ false, /*allowModules*/ false);
        return access.accessibility === SymbolAccessibility.Accessible;
    }

    function isAnySymbolAccessible(symbols: Symbol[] | undefined, enclosingDeclaration: Node | undefined, initialSymbol: Symbol, meaning: SymbolFlags, shouldComputeAliasesToMakeVisible: boolean, allowModules: boolean): SymbolAccessibilityResult | undefined {
        if (!length(symbols)) return;

        let hadAccessibleChain: Symbol | undefined;
        let earlyModuleBail = false;
        for (const symbol of symbols!) {
            // Symbol is accessible if it by itself is accessible
            const accessibleSymbolChain = getAccessibleSymbolChain(symbol, enclosingDeclaration, meaning, /*useOnlyExternalAliasing*/ false);
            if (accessibleSymbolChain) {
                hadAccessibleChain = symbol;
                const hasAccessibleDeclarations = hasVisibleDeclarations(accessibleSymbolChain[0], shouldComputeAliasesToMakeVisible);
                if (hasAccessibleDeclarations) {
                    return hasAccessibleDeclarations;
                }
            }
            if (allowModules) {
                if (some(symbol.declarations, hasNonGlobalAugmentationExternalModuleSymbol)) {
                    if (shouldComputeAliasesToMakeVisible) {
                        earlyModuleBail = true;
                        // Generally speaking, we want to use the aliases that already exist to refer to a module, if present
                        // In order to do so, we need to find those aliases in order to retain them in declaration emit; so
                        // if we are in declaration emit, we cannot use the fast path for module visibility until we've exhausted
                        // all other visibility options (in order to capture the possible aliases used to reference the module)
                        continue;
                    }
                    // Any meaning of a module symbol is always accessible via an `import` type
                    return {
                        accessibility: SymbolAccessibility.Accessible,
                    };
                }
            }

            // If we haven't got the accessible symbol, it doesn't mean the symbol is actually inaccessible.
            // It could be a qualified symbol and hence verify the path
            // e.g.:
            // module m {
            //     export class c {
            //     }
            // }
            // const x: typeof m.c
            // In the above example when we start with checking if typeof m.c symbol is accessible,
            // we are going to see if c can be accessed in scope directly.
            // But it can't, hence the accessible is going to be undefined, but that doesn't mean m.c is inaccessible
            // It is accessible if the parent m is accessible because then m.c can be accessed through qualification

            const containers = getContainersOfSymbol(symbol, enclosingDeclaration, meaning);
            const parentResult = isAnySymbolAccessible(containers, enclosingDeclaration, initialSymbol, initialSymbol === symbol ? getQualifiedLeftMeaning(meaning) : meaning, shouldComputeAliasesToMakeVisible, allowModules);
            if (parentResult) {
                return parentResult;
            }
        }

        if (earlyModuleBail) {
            return {
                accessibility: SymbolAccessibility.Accessible,
            };
        }

        if (hadAccessibleChain) {
            return {
                accessibility: SymbolAccessibility.NotAccessible,
                errorSymbolName: symbolToString(initialSymbol, enclosingDeclaration, meaning),
                errorModuleName: hadAccessibleChain !== initialSymbol ? symbolToString(hadAccessibleChain, enclosingDeclaration, SymbolFlags.Namespace) : undefined,
            };
        }
    }

    /**
     * Check if the given symbol in given enclosing declaration is accessible and mark all associated alias to be visible if requested
     *
     * @param symbol a Symbol to check if accessible
     * @param enclosingDeclaration a Node containing reference to the symbol
     * @param meaning a SymbolFlags to check if such meaning of the symbol is accessible
     * @param shouldComputeAliasToMakeVisible a boolean value to indicate whether to return aliases to be mark visible in case the symbol is accessible
     */
    function isSymbolAccessible(symbol: Symbol | undefined, enclosingDeclaration: Node | undefined, meaning: SymbolFlags, shouldComputeAliasesToMakeVisible: boolean): SymbolAccessibilityResult {
        return isSymbolAccessibleWorker(symbol, enclosingDeclaration, meaning, shouldComputeAliasesToMakeVisible, /*allowModules*/ true);
    }

    function isSymbolAccessibleWorker(symbol: Symbol | undefined, enclosingDeclaration: Node | undefined, meaning: SymbolFlags, shouldComputeAliasesToMakeVisible: boolean, allowModules: boolean): SymbolAccessibilityResult {
        if (symbol && enclosingDeclaration) {
            const result = isAnySymbolAccessible([symbol], enclosingDeclaration, symbol, meaning, shouldComputeAliasesToMakeVisible, allowModules);
            if (result) {
                return result;
            }

            // This could be a symbol that is not exported in the external module
            // or it could be a symbol from different external module that is not aliased and hence cannot be named
            const symbolExternalModule = forEach(symbol.declarations, getExternalModuleContainer);
            if (symbolExternalModule) {
                const enclosingExternalModule = getExternalModuleContainer(enclosingDeclaration);
                if (symbolExternalModule !== enclosingExternalModule) {
                    // name from different external module that is not visible
                    return {
                        accessibility: SymbolAccessibility.CannotBeNamed,
                        errorSymbolName: symbolToString(symbol, enclosingDeclaration, meaning),
                        errorModuleName: symbolToString(symbolExternalModule),
                        errorNode: isInJSFile(enclosingDeclaration) ? enclosingDeclaration : undefined,
                    };
                }
            }

            // Just a local name that is not accessible
            return {
                accessibility: SymbolAccessibility.NotAccessible,
                errorSymbolName: symbolToString(symbol, enclosingDeclaration, meaning),
            };
        }

        return { accessibility: SymbolAccessibility.Accessible };
    }

    function getExternalModuleContainer(declaration: Node) {
        const node = findAncestor(declaration, hasExternalModuleSymbol);
        return node && getSymbolOfDeclaration(node as AmbientModuleDeclaration | SourceFile);
    }

    function hasExternalModuleSymbol(declaration: Node) {
        return isAmbientModule(declaration) || (declaration.kind === SyntaxKind.SourceFile && isExternalOrCommonJsModule(declaration as SourceFile));
    }

    function hasNonGlobalAugmentationExternalModuleSymbol(declaration: Node) {
        return isModuleWithStringLiteralName(declaration) || (declaration.kind === SyntaxKind.SourceFile && isExternalOrCommonJsModule(declaration as SourceFile));
    }

    function hasVisibleDeclarations(symbol: Symbol, shouldComputeAliasToMakeVisible: boolean): SymbolVisibilityResult | undefined {
        let aliasesToMakeVisible: LateVisibilityPaintedStatement[] | undefined;
        if (!every(filter(symbol.declarations, d => d.kind !== SyntaxKind.Identifier), getIsDeclarationVisible)) {
            return undefined;
        }
        return { accessibility: SymbolAccessibility.Accessible, aliasesToMakeVisible };

        function getIsDeclarationVisible(declaration: Declaration) {
            if (!isDeclarationVisible(declaration)) {
                // Mark the unexported alias as visible if its parent is visible
                // because these kind of aliases can be used to name types in declaration file

                const anyImportSyntax = getAnyImportSyntax(declaration);
                if (
                    anyImportSyntax &&
                    !hasSyntacticModifier(anyImportSyntax, ModifierFlags.Export) && // import clause without export
                    isDeclarationVisible(anyImportSyntax.parent)
                ) {
                    return addVisibleAlias(declaration, anyImportSyntax);
                }
                else if (
                    isVariableDeclaration(declaration) && isVariableStatement(declaration.parent.parent) &&
                    !hasSyntacticModifier(declaration.parent.parent, ModifierFlags.Export) && // unexported variable statement
                    isDeclarationVisible(declaration.parent.parent.parent)
                ) {
                    return addVisibleAlias(declaration, declaration.parent.parent);
                }
                else if (
                    isLateVisibilityPaintedStatement(declaration) // unexported top-level statement
                    && !hasSyntacticModifier(declaration, ModifierFlags.Export)
                    && isDeclarationVisible(declaration.parent)
                ) {
                    return addVisibleAlias(declaration, declaration);
                }
                else if (isBindingElement(declaration)) {
                    if (
                        symbol.flags & SymbolFlags.Alias && isInJSFile(declaration) && declaration.parent?.parent // exported import-like top-level JS require statement
                        && isVariableDeclaration(declaration.parent.parent)
                        && declaration.parent.parent.parent?.parent && isVariableStatement(declaration.parent.parent.parent.parent)
                        && !hasSyntacticModifier(declaration.parent.parent.parent.parent, ModifierFlags.Export)
                        && declaration.parent.parent.parent.parent.parent // check if the thing containing the variable statement is visible (ie, the file)
                        && isDeclarationVisible(declaration.parent.parent.parent.parent.parent)
                    ) {
                        return addVisibleAlias(declaration, declaration.parent.parent.parent.parent);
                    }
                    else if (symbol.flags & SymbolFlags.BlockScopedVariable) {
                        const rootDeclaration = walkUpBindingElementsAndPatterns(declaration);
