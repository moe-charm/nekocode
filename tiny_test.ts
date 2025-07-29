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
