#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Data.dll>
#using <System.Xml.dll>

using namespace System::Security::Permissions;
[assembly:SecurityPermissionAttribute(SecurityAction::RequestMinimum, SkipVerification=false)];
// 
// Этот исходный код был создан с помощью xsd, версия=4.6.1055.0.
// 
namespace notepadPlusPlus {
    using namespace System;
    ref class NewDataSet;
    
    
    /// <summary>
///Represents a strongly typed in-memory cache of data.
///</summary>
    [System::Serializable, 
    System::ComponentModel::DesignerCategoryAttribute(L"code"), 
    System::ComponentModel::ToolboxItem(true), 
    System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedDataSetSchema"), 
    System::Xml::Serialization::XmlRootAttribute(L"NewDataSet"), 
    System::ComponentModel::Design::HelpKeywordAttribute(L"vs.data.DataSet")]
    public ref class NewDataSet : public ::System::Data::DataSet {
        public : ref class NotepadPlusDataTable;
        public : ref class functionListDataTable;
        public : ref class associationMapDataTable;
        public : ref class associationDataTable;
        public : ref class parsersDataTable;
        public : ref class parserDataTable;
        public : ref class classRangeDataTable;
        public : ref class classNameDataTable;
        public : ref class nameExprDataTable;
        public : ref class functionDataTable;
        public : ref class functionNameDataTable;
        public : ref class funcNameExprDataTable;
        public : ref class NotepadPlusRow;
        public : ref class functionListRow;
        public : ref class associationMapRow;
        public : ref class associationRow;
        public : ref class parsersRow;
        public : ref class parserRow;
        public : ref class classRangeRow;
        public : ref class classNameRow;
        public : ref class nameExprRow;
        public : ref class functionRow;
        public : ref class functionNameRow;
        public : ref class funcNameExprRow;
        public : ref class NotepadPlusRowChangeEvent;
        public : ref class functionListRowChangeEvent;
        public : ref class associationMapRowChangeEvent;
        public : ref class associationRowChangeEvent;
        public : ref class parsersRowChangeEvent;
        public : ref class parserRowChangeEvent;
        public : ref class classRangeRowChangeEvent;
        public : ref class classNameRowChangeEvent;
        public : ref class nameExprRowChangeEvent;
        public : ref class functionRowChangeEvent;
        public : ref class functionNameRowChangeEvent;
        public : ref class funcNameExprRowChangeEvent;
        
        private: notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  tableNotepadPlus;
        
        private: notepadPlusPlus::NewDataSet::functionListDataTable^  tablefunctionList;
        
        private: notepadPlusPlus::NewDataSet::associationMapDataTable^  tableassociationMap;
        
        private: notepadPlusPlus::NewDataSet::associationDataTable^  tableassociation;
        
        private: notepadPlusPlus::NewDataSet::parsersDataTable^  tableparsers;
        
        private: notepadPlusPlus::NewDataSet::parserDataTable^  tableparser;
        
        private: notepadPlusPlus::NewDataSet::classRangeDataTable^  tableclassRange;
        
        private: notepadPlusPlus::NewDataSet::classNameDataTable^  tableclassName;
        
        private: notepadPlusPlus::NewDataSet::nameExprDataTable^  tablenameExpr;
        
        private: notepadPlusPlus::NewDataSet::functionDataTable^  tablefunction;
        
        private: notepadPlusPlus::NewDataSet::functionNameDataTable^  tablefunctionName;
        
        private: notepadPlusPlus::NewDataSet::funcNameExprDataTable^  tablefuncNameExpr;
        
        private: ::System::Data::DataRelation^  relationNotepadPlus_functionList;
        
        private: ::System::Data::DataRelation^  relationfunctionList_associationMap;
        
        private: ::System::Data::DataRelation^  relationassociationMap_association;
        
        private: ::System::Data::DataRelation^  relationfunctionList_parsers;
        
        private: ::System::Data::DataRelation^  relationparsers_parser;
        
        private: ::System::Data::DataRelation^  relationparser_classRange;
        
        private: ::System::Data::DataRelation^  relationfunction_className;
        
        private: ::System::Data::DataRelation^  relationclassRange_className;
        
        private: ::System::Data::DataRelation^  relationclassName_nameExpr;
        
        private: ::System::Data::DataRelation^  relationfunctionName_nameExpr;
        
        private: ::System::Data::DataRelation^  relationclassRange_function;
        
        private: ::System::Data::DataRelation^  relationparser_function;
        
        private: ::System::Data::DataRelation^  relationfunction_functionName;
        
        private: ::System::Data::DataRelation^  relationfunctionName_funcNameExpr;
        
        private: ::System::Data::SchemaSerializationMode _schemaSerializationMode;
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void NotepadPlusRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void functionListRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::functionListRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void associationMapRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::associationMapRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void associationRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::associationRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void parsersRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::parsersRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void parserRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::parserRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void classRangeRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::classRangeRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void classNameRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::classNameRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void nameExprRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::nameExprRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void functionRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::functionRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void functionNameRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::functionNameRowChangeEvent^  e);
        
        public : [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        delegate System::Void funcNameExprRowChangeEventHandler(::System::Object^  sender, notepadPlusPlus::NewDataSet::funcNameExprRowChangeEvent^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        NewDataSet();
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        NewDataSet(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  NotepadPlus {
            notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::functionListDataTable^  functionList {
            notepadPlusPlus::NewDataSet::functionListDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::associationMapDataTable^  associationMap {
            notepadPlusPlus::NewDataSet::associationMapDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::associationDataTable^  association {
            notepadPlusPlus::NewDataSet::associationDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::parsersDataTable^  parsers {
            notepadPlusPlus::NewDataSet::parsersDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::parserDataTable^  parser {
            notepadPlusPlus::NewDataSet::parserDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::classRangeDataTable^  classRange {
            notepadPlusPlus::NewDataSet::classRangeDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::classNameDataTable^  className {
            notepadPlusPlus::NewDataSet::classNameDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::nameExprDataTable^  nameExpr {
            notepadPlusPlus::NewDataSet::nameExprDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::functionDataTable^  function {
            notepadPlusPlus::NewDataSet::functionDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::functionNameDataTable^  functionName {
            notepadPlusPlus::NewDataSet::functionNameDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::Browsable(false), 
        System::ComponentModel::DesignerSerializationVisibility(::System::ComponentModel::DesignerSerializationVisibility::Content)]
        property notepadPlusPlus::NewDataSet::funcNameExprDataTable^  funcNameExpr {
            notepadPlusPlus::NewDataSet::funcNameExprDataTable^  get();
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::BrowsableAttribute(true), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Visible)]
        virtual property ::System::Data::SchemaSerializationMode SchemaSerializationMode {
            ::System::Data::SchemaSerializationMode get() override;
            System::Void set(::System::Data::SchemaSerializationMode value) override;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataTableCollection^  Tables {
            ::System::Data::DataTableCollection^  get() new;
        }
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
        System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
        System::ComponentModel::DesignerSerializationVisibilityAttribute(::System::ComponentModel::DesignerSerializationVisibility::Hidden)]
        property ::System::Data::DataRelationCollection^  Relations {
            ::System::Data::DataRelationCollection^  get() new;
        }
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Void InitializeDerivedDataSet() override;
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Data::DataSet^  Clone() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Boolean ShouldSerializeTables() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Boolean ShouldSerializeRelations() override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Void ReadXmlSerializable(::System::Xml::XmlReader^  reader) override;
        
        protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        virtual ::System::Xml::Schema::XmlSchema^  GetSchemaSerializable() override;
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitVars();
        
        internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitVars(::System::Boolean initTable);
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void InitClass();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeNotepadPlus();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializefunctionList();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeassociationMap();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeassociation();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeparsers();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeparser();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeclassRange();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializeclassName();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializenameExpr();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializefunction();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializefunctionName();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Boolean ShouldSerializefuncNameExpr();
        
        private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ::System::Void SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e);
        
        public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class NotepadPlusDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnNotepadPlus_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEventHandler^  NotepadPlusRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEventHandler^  NotepadPlusRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEventHandler^  NotepadPlusRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEventHandler^  NotepadPlusRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            NotepadPlusDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            NotepadPlusDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            NotepadPlusDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  NotepadPlus_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::NotepadPlusRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::NotepadPlusRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddNotepadPlusRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::NotepadPlusRow^  AddNotepadPlusRow();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewNotepadPlusRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveNotepadPlusRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class functionListDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnfunctionList_Id;
            
            private: ::System::Data::DataColumn^  columnNotepadPlus_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionListRowChangeEventHandler^  functionListRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionListRowChangeEventHandler^  functionListRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionListRowChangeEventHandler^  functionListRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionListRowChangeEventHandler^  functionListRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionListDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionListDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionListDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionList_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  NotepadPlus_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionListRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::functionListRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddfunctionListRow(notepadPlusPlus::NewDataSet::functionListRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionListRow^  AddfunctionListRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  parentNotepadPlusRowByNotepadPlus_functionList);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionListRow^  NewfunctionListRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovefunctionListRow(notepadPlusPlus::NewDataSet::functionListRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class associationMapDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnassociationMap_Id;
            
            private: ::System::Data::DataColumn^  columnfunctionList_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationMapRowChangeEventHandler^  associationMapRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationMapRowChangeEventHandler^  associationMapRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationMapRowChangeEventHandler^  associationMapRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationMapRowChangeEventHandler^  associationMapRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationMapDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationMapDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationMapDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  associationMap_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionList_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::associationMapRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::associationMapRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddassociationMapRow(notepadPlusPlus::NewDataSet::associationMapRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::associationMapRow^  AddassociationMapRow(notepadPlusPlus::NewDataSet::functionListRow^  parentfunctionListRowByfunctionList_associationMap);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::associationMapRow^  NewassociationMapRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveassociationMapRow(notepadPlusPlus::NewDataSet::associationMapRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class associationDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnid;
            
            private: ::System::Data::DataColumn^  columnlangID;
            
            private: ::System::Data::DataColumn^  columnuserDefinedLangName;
            
            private: ::System::Data::DataColumn^  columnext;
            
            private: ::System::Data::DataColumn^  columnassociation_text;
            
            private: ::System::Data::DataColumn^  columnassociationMap_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationRowChangeEventHandler^  associationRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationRowChangeEventHandler^  associationRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationRowChangeEventHandler^  associationRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::associationRowChangeEventHandler^  associationRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  idColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  langIDColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  userDefinedLangNameColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  extColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  association_textColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  associationMap_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::associationRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::associationRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddassociationRow(notepadPlusPlus::NewDataSet::associationRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::associationRow^  AddassociationRow(
                        System::String^  id, 
                        System::SByte langID, 
                        System::String^  userDefinedLangName, 
                        System::String^  ext, 
                        System::String^  association_text, 
                        notepadPlusPlus::NewDataSet::associationMapRow^  parentassociationMapRowByassociationMap_association);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::associationRow^  NewassociationRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveassociationRow(notepadPlusPlus::NewDataSet::associationRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class parsersDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnparsers_Id;
            
            private: ::System::Data::DataColumn^  columnfunctionList_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parsersRowChangeEventHandler^  parsersRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parsersRowChangeEventHandler^  parsersRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parsersRowChangeEventHandler^  parsersRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parsersRowChangeEventHandler^  parsersRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parsersDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parsersDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parsersDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  parsers_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionList_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parsersRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::parsersRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddparsersRow(notepadPlusPlus::NewDataSet::parsersRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::parsersRow^  AddparsersRow(notepadPlusPlus::NewDataSet::functionListRow^  parentfunctionListRowByfunctionList_parsers);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::parsersRow^  NewparsersRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveparsersRow(notepadPlusPlus::NewDataSet::parsersRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class parserDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnid;
            
            private: ::System::Data::DataColumn^  columndisplayName;
            
            private: ::System::Data::DataColumn^  columncommentExpr;
            
            private: ::System::Data::DataColumn^  columnversion;
            
            private: ::System::Data::DataColumn^  columnparser_Id;
            
            private: ::System::Data::DataColumn^  columnparsers_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parserRowChangeEventHandler^  parserRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parserRowChangeEventHandler^  parserRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parserRowChangeEventHandler^  parserRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::parserRowChangeEventHandler^  parserRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parserDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parserDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parserDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  idColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  displayNameColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  commentExprColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  versionColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  parser_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  parsers_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parserRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::parserRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddparserRow(notepadPlusPlus::NewDataSet::parserRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::parserRow^  AddparserRow(System::String^  id, System::String^  displayName, System::String^  commentExpr, 
                        System::String^  version, notepadPlusPlus::NewDataSet::parsersRow^  parentparsersRowByparsers_parser);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::parserRow^  NewparserRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveparserRow(notepadPlusPlus::NewDataSet::parserRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class classRangeDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnmainExpr;
            
            private: ::System::Data::DataColumn^  columnopenSymbole;
            
            private: ::System::Data::DataColumn^  columncloseSymbole;
            
            private: ::System::Data::DataColumn^  columnopenSymbol;
            
            private: ::System::Data::DataColumn^  columncloseSymbol;
            
            private: ::System::Data::DataColumn^  columnclassRange_Id;
            
            private: ::System::Data::DataColumn^  columnparser_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classRangeRowChangeEventHandler^  classRangeRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classRangeRowChangeEventHandler^  classRangeRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classRangeRowChangeEventHandler^  classRangeRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classRangeRowChangeEventHandler^  classRangeRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classRangeDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classRangeDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classRangeDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  mainExprColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  openSymboleColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  closeSymboleColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  openSymbolColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  closeSymbolColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  classRange_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  parser_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classRangeRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::classRangeRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddclassRangeRow(notepadPlusPlus::NewDataSet::classRangeRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::classRangeRow^  AddclassRangeRow(
                        System::String^  mainExpr, 
                        System::String^  openSymbole, 
                        System::String^  closeSymbole, 
                        System::String^  openSymbol, 
                        System::String^  closeSymbol, 
                        notepadPlusPlus::NewDataSet::parserRow^  parentparserRowByparser_classRange);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::classRangeRow^  NewclassRangeRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveclassRangeRow(notepadPlusPlus::NewDataSet::classRangeRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class classNameDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnclassName_Id;
            
            private: ::System::Data::DataColumn^  columnfunction_Id;
            
            private: ::System::Data::DataColumn^  columnclassRange_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classNameRowChangeEventHandler^  classNameRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classNameRowChangeEventHandler^  classNameRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classNameRowChangeEventHandler^  classNameRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::classNameRowChangeEventHandler^  classNameRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classNameDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classNameDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classNameDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  className_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  function_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  classRange_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classNameRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::classNameRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddclassNameRow(notepadPlusPlus::NewDataSet::classNameRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::classNameRow^  AddclassNameRow(notepadPlusPlus::NewDataSet::functionRow^  parentfunctionRowByfunction_className, 
                        notepadPlusPlus::NewDataSet::classRangeRow^  parentclassRangeRowByclassRange_className);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::classNameRow^  NewclassNameRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemoveclassNameRow(notepadPlusPlus::NewDataSet::classNameRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class nameExprDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnexpr;
            
            private: ::System::Data::DataColumn^  columnnameExpr_text;
            
            private: ::System::Data::DataColumn^  columnclassName_Id;
            
            private: ::System::Data::DataColumn^  columnfunctionName_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::nameExprRowChangeEventHandler^  nameExprRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::nameExprRowChangeEventHandler^  nameExprRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::nameExprRowChangeEventHandler^  nameExprRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::nameExprRowChangeEventHandler^  nameExprRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            nameExprDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            nameExprDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            nameExprDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  exprColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  nameExpr_textColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  className_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionName_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::nameExprRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::nameExprRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddnameExprRow(notepadPlusPlus::NewDataSet::nameExprRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::nameExprRow^  AddnameExprRow(System::String^  expr, System::String^  nameExpr_text, 
                        notepadPlusPlus::NewDataSet::classNameRow^  parentclassNameRowByclassName_nameExpr, notepadPlusPlus::NewDataSet::functionNameRow^  parentfunctionNameRowByfunctionName_nameExpr);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::nameExprRow^  NewnameExprRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovenameExprRow(notepadPlusPlus::NewDataSet::nameExprRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class functionDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnmainExpr;
            
            private: ::System::Data::DataColumn^  columnfunction_Id;
            
            private: ::System::Data::DataColumn^  columnclassRange_Id;
            
            private: ::System::Data::DataColumn^  columnparser_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionRowChangeEventHandler^  functionRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionRowChangeEventHandler^  functionRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionRowChangeEventHandler^  functionRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionRowChangeEventHandler^  functionRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  mainExprColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  function_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  classRange_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  parser_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::functionRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddfunctionRow(notepadPlusPlus::NewDataSet::functionRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionRow^  AddfunctionRow(System::String^  mainExpr, notepadPlusPlus::NewDataSet::classRangeRow^  parentclassRangeRowByclassRange_function, 
                        notepadPlusPlus::NewDataSet::parserRow^  parentparserRowByparser_function);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionRow^  NewfunctionRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovefunctionRow(notepadPlusPlus::NewDataSet::functionRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class functionNameDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnfunctionName_Id;
            
            private: ::System::Data::DataColumn^  columnfunction_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionNameRowChangeEventHandler^  functionNameRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionNameRowChangeEventHandler^  functionNameRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionNameRowChangeEventHandler^  functionNameRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::functionNameRowChangeEventHandler^  functionNameRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionNameDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionNameDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionNameDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionName_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  function_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionNameRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::functionNameRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddfunctionNameRow(notepadPlusPlus::NewDataSet::functionNameRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionNameRow^  AddfunctionNameRow(notepadPlusPlus::NewDataSet::functionRow^  parentfunctionRowByfunction_functionName);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::functionNameRow^  NewfunctionNameRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovefunctionNameRow(notepadPlusPlus::NewDataSet::functionNameRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents the strongly named DataTable class.
///</summary>
        [System::Serializable, 
        System::Xml::Serialization::XmlSchemaProviderAttribute(L"GetTypedTableSchema")]
        ref class funcNameExprDataTable : public ::System::Data::DataTable, public ::System::Collections::IEnumerable {
            
            private: ::System::Data::DataColumn^  columnexpr;
            
            private: ::System::Data::DataColumn^  columnfuncNameExpr_text;
            
            private: ::System::Data::DataColumn^  columnfunctionName_Id;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::funcNameExprRowChangeEventHandler^  funcNameExprRowChanging;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::funcNameExprRowChangeEventHandler^  funcNameExprRowChanged;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::funcNameExprRowChangeEventHandler^  funcNameExprRowDeleting;
            
            public: [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            event notepadPlusPlus::NewDataSet::funcNameExprRowChangeEventHandler^  funcNameExprRowDeleted;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            funcNameExprDataTable();
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            funcNameExprDataTable(::System::Data::DataTable^  table);
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            funcNameExprDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  exprColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  funcNameExpr_textColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataColumn^  functionName_IdColumn {
                ::System::Data::DataColumn^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0"), 
            System::ComponentModel::Browsable(false)]
            property ::System::Int32 Count {
                ::System::Int32 get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::funcNameExprRow^  default [::System::Int32 ] {
                notepadPlusPlus::NewDataSet::funcNameExprRow^  get(::System::Int32 index);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void AddfuncNameExprRow(notepadPlusPlus::NewDataSet::funcNameExprRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::funcNameExprRow^  AddfuncNameExprRow(System::String^  expr, System::String^  funcNameExpr_text, 
                        notepadPlusPlus::NewDataSet::functionNameRow^  parentfunctionNameRowByfunctionName_funcNameExpr);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Collections::IEnumerator^  GetEnumerator();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  Clone() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataTable^  CreateInstance() override;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitVars();
            
            private: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void InitClass();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            notepadPlusPlus::NewDataSet::funcNameExprRow^  NewfuncNameExprRow();
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Data::DataRow^  NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Type^  GetRowType() override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) override;
            
            protected: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            virtual ::System::Void OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) override;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void RemovefuncNameExprRow(notepadPlusPlus::NewDataSet::funcNameExprRow^  row);
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            static ::System::Xml::Schema::XmlSchemaComplexType^  GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs);
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class NotepadPlusRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  tableNotepadPlus;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            NotepadPlusRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 NotepadPlus_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::functionListRow^  >^  GetfunctionListRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class functionListRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::functionListDataTable^  tablefunctionList;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionListRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionList_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 NotepadPlus_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::NotepadPlusRow^  NotepadPlusRow {
                notepadPlusPlus::NewDataSet::NotepadPlusRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::NotepadPlusRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsNotepadPlus_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetNotepadPlus_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::associationMapRow^  >^  GetassociationMapRows();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::parsersRow^  >^  GetparsersRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class associationMapRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::associationMapDataTable^  tableassociationMap;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationMapRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 associationMap_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionList_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionListRow^  functionListRow {
                notepadPlusPlus::NewDataSet::functionListRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionListRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsfunctionList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetfunctionList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::associationRow^  >^  GetassociationRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class associationRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::associationDataTable^  tableassociation;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  id {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::SByte langID {
                System::SByte get();
                System::Void set(System::SByte value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  userDefinedLangName {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  ext {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  association_text {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 associationMap_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::associationMapRow^  associationMapRow {
                notepadPlusPlus::NewDataSet::associationMapRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::associationMapRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IslangIDNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetlangIDNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsuserDefinedLangNameNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetuserDefinedLangNameNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsextNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetextNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsassociationMap_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetassociationMap_IdNull();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class parsersRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::parsersDataTable^  tableparsers;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parsersRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 parsers_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionList_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionListRow^  functionListRow {
                notepadPlusPlus::NewDataSet::functionListRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionListRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsfunctionList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetfunctionList_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::parserRow^  >^  GetparserRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class parserRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::parserDataTable^  tableparser;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parserRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  id {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  displayName {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  commentExpr {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  version {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 parser_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 parsers_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parsersRow^  parsersRow {
                notepadPlusPlus::NewDataSet::parsersRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::parsersRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsdisplayNameNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetdisplayNameNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IscommentExprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetcommentExprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsversionNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetversionNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isparsers_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setparsers_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::classRangeRow^  >^  GetclassRangeRows();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  GetfunctionRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class classRangeRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::classRangeDataTable^  tableclassRange;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classRangeRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  mainExpr {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  openSymbole {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  closeSymbole {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  openSymbol {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  closeSymbol {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 classRange_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 parser_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parserRow^  parserRow {
                notepadPlusPlus::NewDataSet::parserRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::parserRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsopenSymboleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetopenSymboleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IscloseSymboleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetcloseSymboleNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsopenSymbolNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetopenSymbolNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IscloseSymbolNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetcloseSymbolNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isparser_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setparser_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  GetclassNameRows();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  GetfunctionRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class classNameRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::classNameDataTable^  tableclassName;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classNameRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 className_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 function_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 classRange_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionRow^  functionRow {
                notepadPlusPlus::NewDataSet::functionRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classRangeRow^  classRangeRow {
                notepadPlusPlus::NewDataSet::classRangeRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::classRangeRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isfunction_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setfunction_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsclassRange_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetclassRange_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  GetnameExprRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class nameExprRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::nameExprDataTable^  tablenameExpr;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            nameExprRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  expr {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  nameExpr_text {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 className_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionName_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classNameRow^  classNameRow {
                notepadPlusPlus::NewDataSet::classNameRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::classNameRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionNameRow^  functionNameRow {
                notepadPlusPlus::NewDataSet::functionNameRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionNameRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsexprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetexprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsclassName_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetclassName_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsfunctionName_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetfunctionName_IdNull();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class functionRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::functionDataTable^  tablefunction;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  mainExpr {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 function_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 classRange_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 parser_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classRangeRow^  classRangeRow {
                notepadPlusPlus::NewDataSet::classRangeRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::classRangeRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parserRow^  parserRow {
                notepadPlusPlus::NewDataSet::parserRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::parserRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsclassRange_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetclassRange_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isparser_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setparser_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::functionNameRow^  >^  GetfunctionNameRows();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  GetclassNameRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class functionNameRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::functionNameDataTable^  tablefunctionName;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionNameRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionName_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 function_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionRow^  functionRow {
                notepadPlusPlus::NewDataSet::functionRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean Isfunction_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void Setfunction_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::funcNameExprRow^  >^  GetfuncNameExprRows();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  GetnameExprRows();
        };
        
        public : /// <summary>
///Represents strongly named DataRow class.
///</summary>
        ref class funcNameExprRow : public ::System::Data::DataRow {
            
            private: notepadPlusPlus::NewDataSet::funcNameExprDataTable^  tablefuncNameExpr;
            
            internal: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            funcNameExprRow(::System::Data::DataRowBuilder^  rb);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  expr {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::String^  funcNameExpr_text {
                System::String^  get();
                System::Void set(System::String^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property System::Int32 functionName_Id {
                System::Int32 get();
                System::Void set(System::Int32 value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionNameRow^  functionNameRow {
                notepadPlusPlus::NewDataSet::functionNameRow^  get();
                System::Void set(notepadPlusPlus::NewDataSet::functionNameRow^  value);
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsexprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetexprNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Boolean IsfunctionName_IdNull();
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            ::System::Void SetfunctionName_IdNull();
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class NotepadPlusRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::NotepadPlusRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            NotepadPlusRowChangeEvent(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::NotepadPlusRow^  Row {
                notepadPlusPlus::NewDataSet::NotepadPlusRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class functionListRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::functionListRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionListRowChangeEvent(notepadPlusPlus::NewDataSet::functionListRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionListRow^  Row {
                notepadPlusPlus::NewDataSet::functionListRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class associationMapRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::associationMapRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationMapRowChangeEvent(notepadPlusPlus::NewDataSet::associationMapRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::associationMapRow^  Row {
                notepadPlusPlus::NewDataSet::associationMapRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class associationRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::associationRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            associationRowChangeEvent(notepadPlusPlus::NewDataSet::associationRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::associationRow^  Row {
                notepadPlusPlus::NewDataSet::associationRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class parsersRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::parsersRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parsersRowChangeEvent(notepadPlusPlus::NewDataSet::parsersRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parsersRow^  Row {
                notepadPlusPlus::NewDataSet::parsersRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class parserRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::parserRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            parserRowChangeEvent(notepadPlusPlus::NewDataSet::parserRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::parserRow^  Row {
                notepadPlusPlus::NewDataSet::parserRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class classRangeRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::classRangeRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classRangeRowChangeEvent(notepadPlusPlus::NewDataSet::classRangeRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classRangeRow^  Row {
                notepadPlusPlus::NewDataSet::classRangeRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class classNameRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::classNameRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            classNameRowChangeEvent(notepadPlusPlus::NewDataSet::classNameRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::classNameRow^  Row {
                notepadPlusPlus::NewDataSet::classNameRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class nameExprRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::nameExprRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            nameExprRowChangeEvent(notepadPlusPlus::NewDataSet::nameExprRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::nameExprRow^  Row {
                notepadPlusPlus::NewDataSet::nameExprRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class functionRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::functionRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionRowChangeEvent(notepadPlusPlus::NewDataSet::functionRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionRow^  Row {
                notepadPlusPlus::NewDataSet::functionRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class functionNameRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::functionNameRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            functionNameRowChangeEvent(notepadPlusPlus::NewDataSet::functionNameRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::functionNameRow^  Row {
                notepadPlusPlus::NewDataSet::functionNameRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
        
        public : /// <summary>
///Row event argument class
///</summary>
        [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
        ref class funcNameExprRowChangeEvent : public ::System::EventArgs {
            
            private: notepadPlusPlus::NewDataSet::funcNameExprRow^  eventRow;
            
            private: ::System::Data::DataRowAction eventAction;
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute]
            [System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            funcNameExprRowChangeEvent(notepadPlusPlus::NewDataSet::funcNameExprRow^  row, ::System::Data::DataRowAction action);
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property notepadPlusPlus::NewDataSet::funcNameExprRow^  Row {
                notepadPlusPlus::NewDataSet::funcNameExprRow^  get();
            }
            
            public: [System::Diagnostics::DebuggerNonUserCodeAttribute, 
            System::CodeDom::Compiler::GeneratedCodeAttribute(L"System.Data.Design.TypedDataSetGenerator", L"4.0.0.0")]
            property ::System::Data::DataRowAction Action {
                ::System::Data::DataRowAction get();
            }
        };
    };
}
namespace notepadPlusPlus {
    
    
    inline NewDataSet::NewDataSet() {
        this->BeginInit();
        this->InitClass();
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &notepadPlusPlus::NewDataSet::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        __super::Relations->CollectionChanged += schemaChangedHandler;
        this->EndInit();
    }
    
    inline NewDataSet::NewDataSet(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataSet(info, context, false) {
        if (this->IsBinarySerialized(info, context) == true) {
            this->InitVars(false);
            ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler1 = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &notepadPlusPlus::NewDataSet::SchemaChanged);
            this->Tables->CollectionChanged += schemaChangedHandler1;
            this->Relations->CollectionChanged += schemaChangedHandler1;
            return;
        }
        ::System::String^  strSchema = (cli::safe_cast<::System::String^  >(info->GetValue(L"XmlSchema", ::System::String::typeid)));
        if (this->DetermineSchemaSerializationMode(info, context) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
            if (ds->Tables[L"NotepadPlus"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::NotepadPlusDataTable(ds->Tables[L"NotepadPlus"])));
            }
            if (ds->Tables[L"functionList"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionListDataTable(ds->Tables[L"functionList"])));
            }
            if (ds->Tables[L"associationMap"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::associationMapDataTable(ds->Tables[L"associationMap"])));
            }
            if (ds->Tables[L"association"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::associationDataTable(ds->Tables[L"association"])));
            }
            if (ds->Tables[L"parsers"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::parsersDataTable(ds->Tables[L"parsers"])));
            }
            if (ds->Tables[L"parser"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::parserDataTable(ds->Tables[L"parser"])));
            }
            if (ds->Tables[L"classRange"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::classRangeDataTable(ds->Tables[L"classRange"])));
            }
            if (ds->Tables[L"className"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::classNameDataTable(ds->Tables[L"className"])));
            }
            if (ds->Tables[L"nameExpr"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::nameExprDataTable(ds->Tables[L"nameExpr"])));
            }
            if (ds->Tables[L"function"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionDataTable(ds->Tables[L"function"])));
            }
            if (ds->Tables[L"functionName"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionNameDataTable(ds->Tables[L"functionName"])));
            }
            if (ds->Tables[L"funcNameExpr"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::funcNameExprDataTable(ds->Tables[L"funcNameExpr"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXmlSchema((gcnew ::System::Xml::XmlTextReader((gcnew ::System::IO::StringReader(strSchema)))));
        }
        this->GetSerializationData(info, context);
        ::System::ComponentModel::CollectionChangeEventHandler^  schemaChangedHandler = gcnew ::System::ComponentModel::CollectionChangeEventHandler(this, &notepadPlusPlus::NewDataSet::SchemaChanged);
        __super::Tables->CollectionChanged += schemaChangedHandler;
        this->Relations->CollectionChanged += schemaChangedHandler;
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  NewDataSet::NotepadPlus::get() {
        return this->tableNotepadPlus;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListDataTable^  NewDataSet::functionList::get() {
        return this->tablefunctionList;
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapDataTable^  NewDataSet::associationMap::get() {
        return this->tableassociationMap;
    }
    
    inline notepadPlusPlus::NewDataSet::associationDataTable^  NewDataSet::association::get() {
        return this->tableassociation;
    }
    
    inline notepadPlusPlus::NewDataSet::parsersDataTable^  NewDataSet::parsers::get() {
        return this->tableparsers;
    }
    
    inline notepadPlusPlus::NewDataSet::parserDataTable^  NewDataSet::parser::get() {
        return this->tableparser;
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeDataTable^  NewDataSet::classRange::get() {
        return this->tableclassRange;
    }
    
    inline notepadPlusPlus::NewDataSet::classNameDataTable^  NewDataSet::className::get() {
        return this->tableclassName;
    }
    
    inline notepadPlusPlus::NewDataSet::nameExprDataTable^  NewDataSet::nameExpr::get() {
        return this->tablenameExpr;
    }
    
    inline notepadPlusPlus::NewDataSet::functionDataTable^  NewDataSet::function::get() {
        return this->tablefunction;
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameDataTable^  NewDataSet::functionName::get() {
        return this->tablefunctionName;
    }
    
    inline notepadPlusPlus::NewDataSet::funcNameExprDataTable^  NewDataSet::funcNameExpr::get() {
        return this->tablefuncNameExpr;
    }
    
    inline ::System::Data::SchemaSerializationMode NewDataSet::SchemaSerializationMode::get() {
        return this->_schemaSerializationMode;
    }
    inline System::Void NewDataSet::SchemaSerializationMode::set(::System::Data::SchemaSerializationMode value) {
        this->_schemaSerializationMode = __identifier(value);
    }
    
    inline ::System::Data::DataTableCollection^  NewDataSet::Tables::get() {
        return __super::Tables;
    }
    
    inline ::System::Data::DataRelationCollection^  NewDataSet::Relations::get() {
        return __super::Relations;
    }
    
    inline ::System::Void NewDataSet::InitializeDerivedDataSet() {
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline ::System::Data::DataSet^  NewDataSet::Clone() {
        notepadPlusPlus::NewDataSet^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet^  >(__super::Clone()));
        cln->InitVars();
        cln->SchemaSerializationMode = this->SchemaSerializationMode;
        return cln;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeTables() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeRelations() {
        return false;
    }
    
    inline ::System::Void NewDataSet::ReadXmlSerializable(::System::Xml::XmlReader^  reader) {
        if (this->DetermineSchemaSerializationMode(reader) == ::System::Data::SchemaSerializationMode::IncludeSchema) {
            this->Reset();
            ::System::Data::DataSet^  ds = (gcnew ::System::Data::DataSet());
            ds->ReadXml(reader);
            if (ds->Tables[L"NotepadPlus"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::NotepadPlusDataTable(ds->Tables[L"NotepadPlus"])));
            }
            if (ds->Tables[L"functionList"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionListDataTable(ds->Tables[L"functionList"])));
            }
            if (ds->Tables[L"associationMap"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::associationMapDataTable(ds->Tables[L"associationMap"])));
            }
            if (ds->Tables[L"association"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::associationDataTable(ds->Tables[L"association"])));
            }
            if (ds->Tables[L"parsers"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::parsersDataTable(ds->Tables[L"parsers"])));
            }
            if (ds->Tables[L"parser"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::parserDataTable(ds->Tables[L"parser"])));
            }
            if (ds->Tables[L"classRange"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::classRangeDataTable(ds->Tables[L"classRange"])));
            }
            if (ds->Tables[L"className"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::classNameDataTable(ds->Tables[L"className"])));
            }
            if (ds->Tables[L"nameExpr"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::nameExprDataTable(ds->Tables[L"nameExpr"])));
            }
            if (ds->Tables[L"function"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionDataTable(ds->Tables[L"function"])));
            }
            if (ds->Tables[L"functionName"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::functionNameDataTable(ds->Tables[L"functionName"])));
            }
            if (ds->Tables[L"funcNameExpr"] != nullptr) {
                __super::Tables->Add((gcnew notepadPlusPlus::NewDataSet::funcNameExprDataTable(ds->Tables[L"funcNameExpr"])));
            }
            this->DataSetName = ds->DataSetName;
            this->Prefix = ds->Prefix;
            this->Namespace = ds->Namespace;
            this->Locale = ds->Locale;
            this->CaseSensitive = ds->CaseSensitive;
            this->EnforceConstraints = ds->EnforceConstraints;
            this->Merge(ds, false, ::System::Data::MissingSchemaAction::Add);
            this->InitVars();
        }
        else {
            this->ReadXml(reader);
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchema^  NewDataSet::GetSchemaSerializable() {
        ::System::IO::MemoryStream^  stream = (gcnew ::System::IO::MemoryStream());
        this->WriteXmlSchema((gcnew ::System::Xml::XmlTextWriter(stream, nullptr)));
        stream->Position = 0;
        return ::System::Xml::Schema::XmlSchema::Read((gcnew ::System::Xml::XmlTextReader(stream)), nullptr);
    }
    
    inline ::System::Void NewDataSet::InitVars() {
        this->InitVars(true);
    }
    
    inline ::System::Void NewDataSet::InitVars(::System::Boolean initTable) {
        this->tableNotepadPlus = (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  >(__super::Tables[L"NotepadPlus"]));
        if (initTable == true) {
            if (this->tableNotepadPlus != nullptr) {
                this->tableNotepadPlus->InitVars();
            }
        }
        this->tablefunctionList = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListDataTable^  >(__super::Tables[L"functionList"]));
        if (initTable == true) {
            if (this->tablefunctionList != nullptr) {
                this->tablefunctionList->InitVars();
            }
        }
        this->tableassociationMap = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapDataTable^  >(__super::Tables[L"associationMap"]));
        if (initTable == true) {
            if (this->tableassociationMap != nullptr) {
                this->tableassociationMap->InitVars();
            }
        }
        this->tableassociation = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationDataTable^  >(__super::Tables[L"association"]));
        if (initTable == true) {
            if (this->tableassociation != nullptr) {
                this->tableassociation->InitVars();
            }
        }
        this->tableparsers = (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersDataTable^  >(__super::Tables[L"parsers"]));
        if (initTable == true) {
            if (this->tableparsers != nullptr) {
                this->tableparsers->InitVars();
            }
        }
        this->tableparser = (cli::safe_cast<notepadPlusPlus::NewDataSet::parserDataTable^  >(__super::Tables[L"parser"]));
        if (initTable == true) {
            if (this->tableparser != nullptr) {
                this->tableparser->InitVars();
            }
        }
        this->tableclassRange = (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeDataTable^  >(__super::Tables[L"classRange"]));
        if (initTable == true) {
            if (this->tableclassRange != nullptr) {
                this->tableclassRange->InitVars();
            }
        }
        this->tableclassName = (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameDataTable^  >(__super::Tables[L"className"]));
        if (initTable == true) {
            if (this->tableclassName != nullptr) {
                this->tableclassName->InitVars();
            }
        }
        this->tablenameExpr = (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprDataTable^  >(__super::Tables[L"nameExpr"]));
        if (initTable == true) {
            if (this->tablenameExpr != nullptr) {
                this->tablenameExpr->InitVars();
            }
        }
        this->tablefunction = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionDataTable^  >(__super::Tables[L"function"]));
        if (initTable == true) {
            if (this->tablefunction != nullptr) {
                this->tablefunction->InitVars();
            }
        }
        this->tablefunctionName = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameDataTable^  >(__super::Tables[L"functionName"]));
        if (initTable == true) {
            if (this->tablefunctionName != nullptr) {
                this->tablefunctionName->InitVars();
            }
        }
        this->tablefuncNameExpr = (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprDataTable^  >(__super::Tables[L"funcNameExpr"]));
        if (initTable == true) {
            if (this->tablefuncNameExpr != nullptr) {
                this->tablefuncNameExpr->InitVars();
            }
        }
        this->relationNotepadPlus_functionList = this->Relations[L"NotepadPlus_functionList"];
        this->relationfunctionList_associationMap = this->Relations[L"functionList_associationMap"];
        this->relationassociationMap_association = this->Relations[L"associationMap_association"];
        this->relationfunctionList_parsers = this->Relations[L"functionList_parsers"];
        this->relationparsers_parser = this->Relations[L"parsers_parser"];
        this->relationparser_classRange = this->Relations[L"parser_classRange"];
        this->relationfunction_className = this->Relations[L"function_className"];
        this->relationclassRange_className = this->Relations[L"classRange_className"];
        this->relationclassName_nameExpr = this->Relations[L"className_nameExpr"];
        this->relationfunctionName_nameExpr = this->Relations[L"functionName_nameExpr"];
        this->relationclassRange_function = this->Relations[L"classRange_function"];
        this->relationparser_function = this->Relations[L"parser_function"];
        this->relationfunction_functionName = this->Relations[L"function_functionName"];
        this->relationfunctionName_funcNameExpr = this->Relations[L"functionName_funcNameExpr"];
    }
    
    inline ::System::Void NewDataSet::InitClass() {
        this->DataSetName = L"NewDataSet";
        this->Prefix = L"";
        this->Locale = (gcnew ::System::Globalization::CultureInfo(L""));
        this->EnforceConstraints = true;
        this->SchemaSerializationMode = ::System::Data::SchemaSerializationMode::IncludeSchema;
        this->tableNotepadPlus = (gcnew notepadPlusPlus::NewDataSet::NotepadPlusDataTable());
        __super::Tables->Add(this->tableNotepadPlus);
        this->tablefunctionList = (gcnew notepadPlusPlus::NewDataSet::functionListDataTable());
        __super::Tables->Add(this->tablefunctionList);
        this->tableassociationMap = (gcnew notepadPlusPlus::NewDataSet::associationMapDataTable());
        __super::Tables->Add(this->tableassociationMap);
        this->tableassociation = (gcnew notepadPlusPlus::NewDataSet::associationDataTable());
        __super::Tables->Add(this->tableassociation);
        this->tableparsers = (gcnew notepadPlusPlus::NewDataSet::parsersDataTable());
        __super::Tables->Add(this->tableparsers);
        this->tableparser = (gcnew notepadPlusPlus::NewDataSet::parserDataTable());
        __super::Tables->Add(this->tableparser);
        this->tableclassRange = (gcnew notepadPlusPlus::NewDataSet::classRangeDataTable());
        __super::Tables->Add(this->tableclassRange);
        this->tableclassName = (gcnew notepadPlusPlus::NewDataSet::classNameDataTable());
        __super::Tables->Add(this->tableclassName);
        this->tablenameExpr = (gcnew notepadPlusPlus::NewDataSet::nameExprDataTable());
        __super::Tables->Add(this->tablenameExpr);
        this->tablefunction = (gcnew notepadPlusPlus::NewDataSet::functionDataTable());
        __super::Tables->Add(this->tablefunction);
        this->tablefunctionName = (gcnew notepadPlusPlus::NewDataSet::functionNameDataTable());
        __super::Tables->Add(this->tablefunctionName);
        this->tablefuncNameExpr = (gcnew notepadPlusPlus::NewDataSet::funcNameExprDataTable());
        __super::Tables->Add(this->tablefuncNameExpr);
        ::System::Data::ForeignKeyConstraint^  fkc;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"NotepadPlus_functionList", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableNotepadPlus->NotepadPlus_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->NotepadPlus_IdColumn}));
        this->tablefunctionList->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"functionList_associationMap", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->functionList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociationMap->functionList_IdColumn}));
        this->tableassociationMap->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"associationMap_association", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociationMap->associationMap_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociation->associationMap_IdColumn}));
        this->tableassociation->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"functionList_parsers", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->functionList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparsers->functionList_IdColumn}));
        this->tableparsers->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"parsers_parser", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparsers->parsers_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parsers_IdColumn}));
        this->tableparser->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"parser_classRange", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parser_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->parser_IdColumn}));
        this->tableclassRange->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"function_className", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->function_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->function_IdColumn}));
        this->tableclassName->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"classRange_className", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->classRange_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->classRange_IdColumn}));
        this->tableclassName->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"className_nameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->className_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablenameExpr->className_IdColumn}));
        this->tablenameExpr->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"functionName_nameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->functionName_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablenameExpr->functionName_IdColumn}));
        this->tablenameExpr->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"classRange_function", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->classRange_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->classRange_IdColumn}));
        this->tablefunction->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"parser_function", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parser_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->parser_IdColumn}));
        this->tablefunction->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"function_functionName", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->function_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->function_IdColumn}));
        this->tablefunctionName->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        fkc = (gcnew ::System::Data::ForeignKeyConstraint(L"functionName_funcNameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->functionName_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefuncNameExpr->functionName_IdColumn}));
        this->tablefuncNameExpr->Constraints->Add(fkc);
        fkc->AcceptRejectRule = ::System::Data::AcceptRejectRule::None;
        fkc->DeleteRule = ::System::Data::Rule::Cascade;
        fkc->UpdateRule = ::System::Data::Rule::Cascade;
        this->relationNotepadPlus_functionList = (gcnew ::System::Data::DataRelation(L"NotepadPlus_functionList", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableNotepadPlus->NotepadPlus_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->NotepadPlus_IdColumn}, false));
        this->relationNotepadPlus_functionList->Nested = true;
        this->Relations->Add(this->relationNotepadPlus_functionList);
        this->relationfunctionList_associationMap = (gcnew ::System::Data::DataRelation(L"functionList_associationMap", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->functionList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociationMap->functionList_IdColumn}, false));
        this->relationfunctionList_associationMap->Nested = true;
        this->Relations->Add(this->relationfunctionList_associationMap);
        this->relationassociationMap_association = (gcnew ::System::Data::DataRelation(L"associationMap_association", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociationMap->associationMap_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableassociation->associationMap_IdColumn}, false));
        this->relationassociationMap_association->Nested = true;
        this->Relations->Add(this->relationassociationMap_association);
        this->relationfunctionList_parsers = (gcnew ::System::Data::DataRelation(L"functionList_parsers", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionList->functionList_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparsers->functionList_IdColumn}, false));
        this->relationfunctionList_parsers->Nested = true;
        this->Relations->Add(this->relationfunctionList_parsers);
        this->relationparsers_parser = (gcnew ::System::Data::DataRelation(L"parsers_parser", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparsers->parsers_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parsers_IdColumn}, false));
        this->relationparsers_parser->Nested = true;
        this->Relations->Add(this->relationparsers_parser);
        this->relationparser_classRange = (gcnew ::System::Data::DataRelation(L"parser_classRange", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parser_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->parser_IdColumn}, false));
        this->relationparser_classRange->Nested = true;
        this->Relations->Add(this->relationparser_classRange);
        this->relationfunction_className = (gcnew ::System::Data::DataRelation(L"function_className", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->function_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->function_IdColumn}, false));
        this->relationfunction_className->Nested = true;
        this->Relations->Add(this->relationfunction_className);
        this->relationclassRange_className = (gcnew ::System::Data::DataRelation(L"classRange_className", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->classRange_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->classRange_IdColumn}, false));
        this->relationclassRange_className->Nested = true;
        this->Relations->Add(this->relationclassRange_className);
        this->relationclassName_nameExpr = (gcnew ::System::Data::DataRelation(L"className_nameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassName->className_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablenameExpr->className_IdColumn}, false));
        this->relationclassName_nameExpr->Nested = true;
        this->Relations->Add(this->relationclassName_nameExpr);
        this->relationfunctionName_nameExpr = (gcnew ::System::Data::DataRelation(L"functionName_nameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->functionName_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablenameExpr->functionName_IdColumn}, false));
        this->relationfunctionName_nameExpr->Nested = true;
        this->Relations->Add(this->relationfunctionName_nameExpr);
        this->relationclassRange_function = (gcnew ::System::Data::DataRelation(L"classRange_function", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableclassRange->classRange_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->classRange_IdColumn}, false));
        this->relationclassRange_function->Nested = true;
        this->Relations->Add(this->relationclassRange_function);
        this->relationparser_function = (gcnew ::System::Data::DataRelation(L"parser_function", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tableparser->parser_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->parser_IdColumn}, false));
        this->relationparser_function->Nested = true;
        this->Relations->Add(this->relationparser_function);
        this->relationfunction_functionName = (gcnew ::System::Data::DataRelation(L"function_functionName", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunction->function_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->function_IdColumn}, false));
        this->relationfunction_functionName->Nested = true;
        this->Relations->Add(this->relationfunction_functionName);
        this->relationfunctionName_funcNameExpr = (gcnew ::System::Data::DataRelation(L"functionName_funcNameExpr", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefunctionName->functionName_IdColumn}, 
            gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->tablefuncNameExpr->functionName_IdColumn}, false));
        this->relationfunctionName_funcNameExpr->Nested = true;
        this->Relations->Add(this->relationfunctionName_funcNameExpr);
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeNotepadPlus() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializefunctionList() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeassociationMap() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeassociation() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeparsers() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeparser() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeclassRange() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializeclassName() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializenameExpr() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializefunction() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializefunctionName() {
        return false;
    }
    
    inline ::System::Boolean NewDataSet::ShouldSerializefuncNameExpr() {
        return false;
    }
    
    inline ::System::Void NewDataSet::SchemaChanged(::System::Object^  sender, ::System::ComponentModel::CollectionChangeEventArgs^  e) {
        if (e->Action == ::System::ComponentModel::CollectionChangeAction::Remove) {
            this->InitVars();
        }
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::GetTypedDataSetSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        ::System::Xml::Schema::XmlSchemaAny^  any = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any->Namespace = ds->Namespace;
        sequence->Items->Add(any);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::NotepadPlusDataTable::NotepadPlusDataTable() {
        this->TableName = L"NotepadPlus";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::NotepadPlusDataTable::NotepadPlusDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::NotepadPlusDataTable::NotepadPlusDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::NotepadPlusDataTable::NotepadPlus_IdColumn::get() {
        return this->columnNotepadPlus_Id;
    }
    
    inline ::System::Int32 NewDataSet::NotepadPlusDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewDataSet::NotepadPlusDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::AddNotepadPlusRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewDataSet::NotepadPlusDataTable::AddNotepadPlusRow() {
        notepadPlusPlus::NewDataSet::NotepadPlusRow^  rowNotepadPlusRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(1) {nullptr};
        rowNotepadPlusRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowNotepadPlusRow);
        return rowNotepadPlusRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::NotepadPlusDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::NotepadPlusDataTable::Clone() {
        notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::NotepadPlusDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::NotepadPlusDataTable());
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::InitVars() {
        this->columnNotepadPlus_Id = __super::Columns[L"NotepadPlus_Id"];
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::InitClass() {
        this->columnNotepadPlus_Id = (gcnew ::System::Data::DataColumn(L"NotepadPlus_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnNotepadPlus_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnNotepadPlus_Id}, 
                true)));
        this->columnNotepadPlus_Id->AutoIncrement = true;
        this->columnNotepadPlus_Id->AllowDBNull = false;
        this->columnNotepadPlus_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewDataSet::NotepadPlusDataTable::NewNotepadPlusRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::NotepadPlusDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::NotepadPlusRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::NotepadPlusDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::NotepadPlusRow::typeid;
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->NotepadPlusRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->NotepadPlusRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->NotepadPlusRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->NotepadPlusRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::NotepadPlusRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::NotepadPlusDataTable::RemoveNotepadPlusRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::NotepadPlusDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"NotepadPlusDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::functionListDataTable::functionListDataTable() {
        this->TableName = L"functionList";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::functionListDataTable::functionListDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::functionListDataTable::functionListDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionListDataTable::functionList_IdColumn::get() {
        return this->columnfunctionList_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionListDataTable::NotepadPlus_IdColumn::get() {
        return this->columnNotepadPlus_Id;
    }
    
    inline ::System::Int32 NewDataSet::functionListDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::functionListDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::AddfunctionListRow(notepadPlusPlus::NewDataSet::functionListRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::functionListDataTable::AddfunctionListRow(notepadPlusPlus::NewDataSet::NotepadPlusRow^  parentNotepadPlusRowByNotepadPlus_functionList) {
        notepadPlusPlus::NewDataSet::functionListRow^  rowfunctionListRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(2) {nullptr, nullptr};
        if (parentNotepadPlusRowByNotepadPlus_functionList != nullptr) {
            columnValuesArray[1] = parentNotepadPlusRowByNotepadPlus_functionList[0];
        }
        rowfunctionListRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowfunctionListRow);
        return rowfunctionListRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::functionListDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionListDataTable::Clone() {
        notepadPlusPlus::NewDataSet::functionListDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionListDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::functionListDataTable());
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::InitVars() {
        this->columnfunctionList_Id = __super::Columns[L"functionList_Id"];
        this->columnNotepadPlus_Id = __super::Columns[L"NotepadPlus_Id"];
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::InitClass() {
        this->columnfunctionList_Id = (gcnew ::System::Data::DataColumn(L"functionList_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionList_Id);
        this->columnNotepadPlus_Id = (gcnew ::System::Data::DataColumn(L"NotepadPlus_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnNotepadPlus_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnfunctionList_Id}, 
                true)));
        this->columnfunctionList_Id->AutoIncrement = true;
        this->columnfunctionList_Id->AllowDBNull = false;
        this->columnfunctionList_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::functionListDataTable::NewfunctionListRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::functionListDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::functionListRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::functionListDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::functionListRow::typeid;
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->functionListRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::functionListRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->functionListRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::functionListRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->functionListRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::functionListRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->functionListRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::functionListRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionListDataTable::RemovefunctionListRow(notepadPlusPlus::NewDataSet::functionListRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::functionListDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"functionListDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::associationMapDataTable::associationMapDataTable() {
        this->TableName = L"associationMap";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::associationMapDataTable::associationMapDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::associationMapDataTable::associationMapDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationMapDataTable::associationMap_IdColumn::get() {
        return this->columnassociationMap_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationMapDataTable::functionList_IdColumn::get() {
        return this->columnfunctionList_Id;
    }
    
    inline ::System::Int32 NewDataSet::associationMapDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapRow^  NewDataSet::associationMapDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::AddassociationMapRow(notepadPlusPlus::NewDataSet::associationMapRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapRow^  NewDataSet::associationMapDataTable::AddassociationMapRow(notepadPlusPlus::NewDataSet::functionListRow^  parentfunctionListRowByfunctionList_associationMap) {
        notepadPlusPlus::NewDataSet::associationMapRow^  rowassociationMapRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(2) {nullptr, nullptr};
        if (parentfunctionListRowByfunctionList_associationMap != nullptr) {
            columnValuesArray[1] = parentfunctionListRowByfunctionList_associationMap[0];
        }
        rowassociationMapRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowassociationMapRow);
        return rowassociationMapRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::associationMapDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::associationMapDataTable::Clone() {
        notepadPlusPlus::NewDataSet::associationMapDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::associationMapDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::associationMapDataTable());
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::InitVars() {
        this->columnassociationMap_Id = __super::Columns[L"associationMap_Id"];
        this->columnfunctionList_Id = __super::Columns[L"functionList_Id"];
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::InitClass() {
        this->columnassociationMap_Id = (gcnew ::System::Data::DataColumn(L"associationMap_Id", ::System::Int32::typeid, nullptr, 
            ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnassociationMap_Id);
        this->columnfunctionList_Id = (gcnew ::System::Data::DataColumn(L"functionList_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionList_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnassociationMap_Id}, 
                true)));
        this->columnassociationMap_Id->AutoIncrement = true;
        this->columnassociationMap_Id->AllowDBNull = false;
        this->columnassociationMap_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapRow^  NewDataSet::associationMapDataTable::NewassociationMapRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::associationMapDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::associationMapRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::associationMapDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::associationMapRow::typeid;
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->associationMapRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::associationMapRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->associationMapRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::associationMapRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->associationMapRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::associationMapRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->associationMapRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::associationMapRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationMapDataTable::RemoveassociationMapRow(notepadPlusPlus::NewDataSet::associationMapRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::associationMapDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"associationMapDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::associationDataTable::associationDataTable() {
        this->TableName = L"association";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::associationDataTable::associationDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::associationDataTable::associationDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::idColumn::get() {
        return this->columnid;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::langIDColumn::get() {
        return this->columnlangID;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::userDefinedLangNameColumn::get() {
        return this->columnuserDefinedLangName;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::extColumn::get() {
        return this->columnext;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::association_textColumn::get() {
        return this->columnassociation_text;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::associationDataTable::associationMap_IdColumn::get() {
        return this->columnassociationMap_Id;
    }
    
    inline ::System::Int32 NewDataSet::associationDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::associationRow^  NewDataSet::associationDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::associationDataTable::AddassociationRow(notepadPlusPlus::NewDataSet::associationRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::associationRow^  NewDataSet::associationDataTable::AddassociationRow(
                System::String^  id, 
                System::SByte langID, 
                System::String^  userDefinedLangName, 
                System::String^  ext, 
                System::String^  association_text, 
                notepadPlusPlus::NewDataSet::associationMapRow^  parentassociationMapRowByassociationMap_association) {
        notepadPlusPlus::NewDataSet::associationRow^  rowassociationRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(6) {id, langID, userDefinedLangName, 
            ext, association_text, nullptr};
        if (parentassociationMapRowByassociationMap_association != nullptr) {
            columnValuesArray[5] = parentassociationMapRowByassociationMap_association[0];
        }
        rowassociationRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowassociationRow);
        return rowassociationRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::associationDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::associationDataTable::Clone() {
        notepadPlusPlus::NewDataSet::associationDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::associationDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::associationDataTable());
    }
    
    inline ::System::Void NewDataSet::associationDataTable::InitVars() {
        this->columnid = __super::Columns[L"id"];
        this->columnlangID = __super::Columns[L"langID"];
        this->columnuserDefinedLangName = __super::Columns[L"userDefinedLangName"];
        this->columnext = __super::Columns[L"ext"];
        this->columnassociation_text = __super::Columns[L"association_text"];
        this->columnassociationMap_Id = __super::Columns[L"associationMap_Id"];
    }
    
    inline ::System::Void NewDataSet::associationDataTable::InitClass() {
        this->columnid = (gcnew ::System::Data::DataColumn(L"id", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnid);
        this->columnlangID = (gcnew ::System::Data::DataColumn(L"langID", ::System::SByte::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnlangID);
        this->columnuserDefinedLangName = (gcnew ::System::Data::DataColumn(L"userDefinedLangName", ::System::String::typeid, nullptr, 
            ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnuserDefinedLangName);
        this->columnext = (gcnew ::System::Data::DataColumn(L"ext", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnext);
        this->columnassociation_text = (gcnew ::System::Data::DataColumn(L"association_text", ::System::String::typeid, nullptr, 
            ::System::Data::MappingType::SimpleContent));
        __super::Columns->Add(this->columnassociation_text);
        this->columnassociationMap_Id = (gcnew ::System::Data::DataColumn(L"associationMap_Id", ::System::Int32::typeid, nullptr, 
            ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnassociationMap_Id);
        this->columnid->AllowDBNull = false;
        this->columnid->Namespace = L"";
        this->columnlangID->Namespace = L"";
        this->columnuserDefinedLangName->Namespace = L"";
        this->columnext->Namespace = L"";
        this->columnassociation_text->AllowDBNull = false;
    }
    
    inline notepadPlusPlus::NewDataSet::associationRow^  NewDataSet::associationDataTable::NewassociationRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::associationDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::associationRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::associationDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::associationRow::typeid;
    }
    
    inline ::System::Void NewDataSet::associationDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->associationRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::associationRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->associationRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::associationRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->associationRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::associationRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->associationRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::associationRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::associationRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::associationDataTable::RemoveassociationRow(notepadPlusPlus::NewDataSet::associationRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::associationDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"associationDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::parsersDataTable::parsersDataTable() {
        this->TableName = L"parsers";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::parsersDataTable::parsersDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::parsersDataTable::parsersDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parsersDataTable::parsers_IdColumn::get() {
        return this->columnparsers_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parsersDataTable::functionList_IdColumn::get() {
        return this->columnfunctionList_Id;
    }
    
    inline ::System::Int32 NewDataSet::parsersDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::parsersRow^  NewDataSet::parsersDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::AddparsersRow(notepadPlusPlus::NewDataSet::parsersRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::parsersRow^  NewDataSet::parsersDataTable::AddparsersRow(notepadPlusPlus::NewDataSet::functionListRow^  parentfunctionListRowByfunctionList_parsers) {
        notepadPlusPlus::NewDataSet::parsersRow^  rowparsersRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(2) {nullptr, nullptr};
        if (parentfunctionListRowByfunctionList_parsers != nullptr) {
            columnValuesArray[1] = parentfunctionListRowByfunctionList_parsers[0];
        }
        rowparsersRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowparsersRow);
        return rowparsersRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::parsersDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::parsersDataTable::Clone() {
        notepadPlusPlus::NewDataSet::parsersDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::parsersDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::parsersDataTable());
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::InitVars() {
        this->columnparsers_Id = __super::Columns[L"parsers_Id"];
        this->columnfunctionList_Id = __super::Columns[L"functionList_Id"];
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::InitClass() {
        this->columnparsers_Id = (gcnew ::System::Data::DataColumn(L"parsers_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnparsers_Id);
        this->columnfunctionList_Id = (gcnew ::System::Data::DataColumn(L"functionList_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionList_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnparsers_Id}, 
                true)));
        this->columnparsers_Id->AutoIncrement = true;
        this->columnparsers_Id->AllowDBNull = false;
        this->columnparsers_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::parsersRow^  NewDataSet::parsersDataTable::NewparsersRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::parsersDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::parsersRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::parsersDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::parsersRow::typeid;
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->parsersRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::parsersRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->parsersRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::parsersRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->parsersRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::parsersRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->parsersRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::parsersRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parsersDataTable::RemoveparsersRow(notepadPlusPlus::NewDataSet::parsersRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::parsersDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"parsersDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::parserDataTable::parserDataTable() {
        this->TableName = L"parser";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::parserDataTable::parserDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::parserDataTable::parserDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::idColumn::get() {
        return this->columnid;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::displayNameColumn::get() {
        return this->columndisplayName;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::commentExprColumn::get() {
        return this->columncommentExpr;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::versionColumn::get() {
        return this->columnversion;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::parser_IdColumn::get() {
        return this->columnparser_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::parserDataTable::parsers_IdColumn::get() {
        return this->columnparsers_Id;
    }
    
    inline ::System::Int32 NewDataSet::parserDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::parserDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::parserDataTable::AddparserRow(notepadPlusPlus::NewDataSet::parserRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::parserDataTable::AddparserRow(System::String^  id, System::String^  displayName, 
                System::String^  commentExpr, System::String^  version, notepadPlusPlus::NewDataSet::parsersRow^  parentparsersRowByparsers_parser) {
        notepadPlusPlus::NewDataSet::parserRow^  rowparserRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(6) {id, displayName, commentExpr, 
            version, nullptr, nullptr};
        if (parentparsersRowByparsers_parser != nullptr) {
            columnValuesArray[5] = parentparsersRowByparsers_parser[0];
        }
        rowparserRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowparserRow);
        return rowparserRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::parserDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::parserDataTable::Clone() {
        notepadPlusPlus::NewDataSet::parserDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::parserDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::parserDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::parserDataTable());
    }
    
    inline ::System::Void NewDataSet::parserDataTable::InitVars() {
        this->columnid = __super::Columns[L"id"];
        this->columndisplayName = __super::Columns[L"displayName"];
        this->columncommentExpr = __super::Columns[L"commentExpr"];
        this->columnversion = __super::Columns[L"version"];
        this->columnparser_Id = __super::Columns[L"parser_Id"];
        this->columnparsers_Id = __super::Columns[L"parsers_Id"];
    }
    
    inline ::System::Void NewDataSet::parserDataTable::InitClass() {
        this->columnid = (gcnew ::System::Data::DataColumn(L"id", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnid);
        this->columndisplayName = (gcnew ::System::Data::DataColumn(L"displayName", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columndisplayName);
        this->columncommentExpr = (gcnew ::System::Data::DataColumn(L"commentExpr", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columncommentExpr);
        this->columnversion = (gcnew ::System::Data::DataColumn(L"version", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnversion);
        this->columnparser_Id = (gcnew ::System::Data::DataColumn(L"parser_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnparser_Id);
        this->columnparsers_Id = (gcnew ::System::Data::DataColumn(L"parsers_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnparsers_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnparser_Id}, 
                true)));
        this->columnid->AllowDBNull = false;
        this->columnid->Namespace = L"";
        this->columndisplayName->Namespace = L"";
        this->columncommentExpr->Namespace = L"";
        this->columnversion->Namespace = L"";
        this->columnparser_Id->AutoIncrement = true;
        this->columnparser_Id->AllowDBNull = false;
        this->columnparser_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::parserDataTable::NewparserRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::parserDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::parserRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::parserDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::parserRow::typeid;
    }
    
    inline ::System::Void NewDataSet::parserDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->parserRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::parserRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parserDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->parserRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::parserRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parserDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->parserRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::parserRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parserDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->parserRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::parserRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::parserDataTable::RemoveparserRow(notepadPlusPlus::NewDataSet::parserRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::parserDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"parserDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::classRangeDataTable::classRangeDataTable() {
        this->TableName = L"classRange";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::classRangeDataTable::classRangeDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::classRangeDataTable::classRangeDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::mainExprColumn::get() {
        return this->columnmainExpr;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::openSymboleColumn::get() {
        return this->columnopenSymbole;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::closeSymboleColumn::get() {
        return this->columncloseSymbole;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::openSymbolColumn::get() {
        return this->columnopenSymbol;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::closeSymbolColumn::get() {
        return this->columncloseSymbol;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::classRange_IdColumn::get() {
        return this->columnclassRange_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classRangeDataTable::parser_IdColumn::get() {
        return this->columnparser_Id;
    }
    
    inline ::System::Int32 NewDataSet::classRangeDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::classRangeDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::AddclassRangeRow(notepadPlusPlus::NewDataSet::classRangeRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::classRangeDataTable::AddclassRangeRow(
                System::String^  mainExpr, 
                System::String^  openSymbole, 
                System::String^  closeSymbole, 
                System::String^  openSymbol, 
                System::String^  closeSymbol, 
                notepadPlusPlus::NewDataSet::parserRow^  parentparserRowByparser_classRange) {
        notepadPlusPlus::NewDataSet::classRangeRow^  rowclassRangeRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(7) {mainExpr, openSymbole, 
            closeSymbole, openSymbol, closeSymbol, nullptr, nullptr};
        if (parentparserRowByparser_classRange != nullptr) {
            columnValuesArray[6] = parentparserRowByparser_classRange[4];
        }
        rowclassRangeRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowclassRangeRow);
        return rowclassRangeRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::classRangeDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::classRangeDataTable::Clone() {
        notepadPlusPlus::NewDataSet::classRangeDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::classRangeDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::classRangeDataTable());
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::InitVars() {
        this->columnmainExpr = __super::Columns[L"mainExpr"];
        this->columnopenSymbole = __super::Columns[L"openSymbole"];
        this->columncloseSymbole = __super::Columns[L"closeSymbole"];
        this->columnopenSymbol = __super::Columns[L"openSymbol"];
        this->columncloseSymbol = __super::Columns[L"closeSymbol"];
        this->columnclassRange_Id = __super::Columns[L"classRange_Id"];
        this->columnparser_Id = __super::Columns[L"parser_Id"];
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::InitClass() {
        this->columnmainExpr = (gcnew ::System::Data::DataColumn(L"mainExpr", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnmainExpr);
        this->columnopenSymbole = (gcnew ::System::Data::DataColumn(L"openSymbole", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnopenSymbole);
        this->columncloseSymbole = (gcnew ::System::Data::DataColumn(L"closeSymbole", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columncloseSymbole);
        this->columnopenSymbol = (gcnew ::System::Data::DataColumn(L"openSymbol", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnopenSymbol);
        this->columncloseSymbol = (gcnew ::System::Data::DataColumn(L"closeSymbol", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columncloseSymbol);
        this->columnclassRange_Id = (gcnew ::System::Data::DataColumn(L"classRange_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnclassRange_Id);
        this->columnparser_Id = (gcnew ::System::Data::DataColumn(L"parser_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnparser_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnclassRange_Id}, 
                true)));
        this->columnmainExpr->AllowDBNull = false;
        this->columnmainExpr->Namespace = L"";
        this->columnopenSymbole->Namespace = L"";
        this->columncloseSymbole->Namespace = L"";
        this->columnopenSymbol->Namespace = L"";
        this->columncloseSymbol->Namespace = L"";
        this->columnclassRange_Id->AutoIncrement = true;
        this->columnclassRange_Id->AllowDBNull = false;
        this->columnclassRange_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::classRangeDataTable::NewclassRangeRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::classRangeDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::classRangeRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::classRangeDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::classRangeRow::typeid;
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->classRangeRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::classRangeRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->classRangeRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::classRangeRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->classRangeRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::classRangeRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->classRangeRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::classRangeRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classRangeDataTable::RemoveclassRangeRow(notepadPlusPlus::NewDataSet::classRangeRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::classRangeDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"classRangeDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::classNameDataTable::classNameDataTable() {
        this->TableName = L"className";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::classNameDataTable::classNameDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::classNameDataTable::classNameDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classNameDataTable::className_IdColumn::get() {
        return this->columnclassName_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classNameDataTable::function_IdColumn::get() {
        return this->columnfunction_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::classNameDataTable::classRange_IdColumn::get() {
        return this->columnclassRange_Id;
    }
    
    inline ::System::Int32 NewDataSet::classNameDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::classNameRow^  NewDataSet::classNameDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::AddclassNameRow(notepadPlusPlus::NewDataSet::classNameRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::classNameRow^  NewDataSet::classNameDataTable::AddclassNameRow(notepadPlusPlus::NewDataSet::functionRow^  parentfunctionRowByfunction_className, 
                notepadPlusPlus::NewDataSet::classRangeRow^  parentclassRangeRowByclassRange_className) {
        notepadPlusPlus::NewDataSet::classNameRow^  rowclassNameRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(3) {nullptr, nullptr, nullptr};
        if (parentfunctionRowByfunction_className != nullptr) {
            columnValuesArray[1] = parentfunctionRowByfunction_className[1];
        }
        if (parentclassRangeRowByclassRange_className != nullptr) {
            columnValuesArray[2] = parentclassRangeRowByclassRange_className[5];
        }
        rowclassNameRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowclassNameRow);
        return rowclassNameRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::classNameDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::classNameDataTable::Clone() {
        notepadPlusPlus::NewDataSet::classNameDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::classNameDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::classNameDataTable());
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::InitVars() {
        this->columnclassName_Id = __super::Columns[L"className_Id"];
        this->columnfunction_Id = __super::Columns[L"function_Id"];
        this->columnclassRange_Id = __super::Columns[L"classRange_Id"];
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::InitClass() {
        this->columnclassName_Id = (gcnew ::System::Data::DataColumn(L"className_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnclassName_Id);
        this->columnfunction_Id = (gcnew ::System::Data::DataColumn(L"function_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunction_Id);
        this->columnclassRange_Id = (gcnew ::System::Data::DataColumn(L"classRange_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnclassRange_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnclassName_Id}, 
                true)));
        this->columnclassName_Id->AutoIncrement = true;
        this->columnclassName_Id->AllowDBNull = false;
        this->columnclassName_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::classNameRow^  NewDataSet::classNameDataTable::NewclassNameRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::classNameDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::classNameRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::classNameDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::classNameRow::typeid;
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->classNameRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::classNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->classNameRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::classNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->classNameRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::classNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->classNameRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::classNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::classNameDataTable::RemoveclassNameRow(notepadPlusPlus::NewDataSet::classNameRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::classNameDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"classNameDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::nameExprDataTable::nameExprDataTable() {
        this->TableName = L"nameExpr";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::nameExprDataTable::nameExprDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::nameExprDataTable::nameExprDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::nameExprDataTable::exprColumn::get() {
        return this->columnexpr;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::nameExprDataTable::nameExpr_textColumn::get() {
        return this->columnnameExpr_text;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::nameExprDataTable::className_IdColumn::get() {
        return this->columnclassName_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::nameExprDataTable::functionName_IdColumn::get() {
        return this->columnfunctionName_Id;
    }
    
    inline ::System::Int32 NewDataSet::nameExprDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::nameExprRow^  NewDataSet::nameExprDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::AddnameExprRow(notepadPlusPlus::NewDataSet::nameExprRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::nameExprRow^  NewDataSet::nameExprDataTable::AddnameExprRow(System::String^  expr, 
                System::String^  nameExpr_text, notepadPlusPlus::NewDataSet::classNameRow^  parentclassNameRowByclassName_nameExpr, notepadPlusPlus::NewDataSet::functionNameRow^  parentfunctionNameRowByfunctionName_nameExpr) {
        notepadPlusPlus::NewDataSet::nameExprRow^  rownameExprRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(4) {expr, nameExpr_text, 
            nullptr, nullptr};
        if (parentclassNameRowByclassName_nameExpr != nullptr) {
            columnValuesArray[2] = parentclassNameRowByclassName_nameExpr[0];
        }
        if (parentfunctionNameRowByfunctionName_nameExpr != nullptr) {
            columnValuesArray[3] = parentfunctionNameRowByfunctionName_nameExpr[0];
        }
        rownameExprRow->ItemArray = columnValuesArray;
        this->Rows->Add(rownameExprRow);
        return rownameExprRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::nameExprDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::nameExprDataTable::Clone() {
        notepadPlusPlus::NewDataSet::nameExprDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::nameExprDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::nameExprDataTable());
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::InitVars() {
        this->columnexpr = __super::Columns[L"expr"];
        this->columnnameExpr_text = __super::Columns[L"nameExpr_text"];
        this->columnclassName_Id = __super::Columns[L"className_Id"];
        this->columnfunctionName_Id = __super::Columns[L"functionName_Id"];
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::InitClass() {
        this->columnexpr = (gcnew ::System::Data::DataColumn(L"expr", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnexpr);
        this->columnnameExpr_text = (gcnew ::System::Data::DataColumn(L"nameExpr_text", ::System::String::typeid, nullptr, ::System::Data::MappingType::SimpleContent));
        __super::Columns->Add(this->columnnameExpr_text);
        this->columnclassName_Id = (gcnew ::System::Data::DataColumn(L"className_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnclassName_Id);
        this->columnfunctionName_Id = (gcnew ::System::Data::DataColumn(L"functionName_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionName_Id);
        this->columnexpr->Namespace = L"";
        this->columnnameExpr_text->AllowDBNull = false;
    }
    
    inline notepadPlusPlus::NewDataSet::nameExprRow^  NewDataSet::nameExprDataTable::NewnameExprRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::nameExprDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::nameExprRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::nameExprDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::nameExprRow::typeid;
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->nameExprRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::nameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->nameExprRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::nameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->nameExprRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::nameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->nameExprRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::nameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::nameExprDataTable::RemovenameExprRow(notepadPlusPlus::NewDataSet::nameExprRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::nameExprDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"nameExprDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::functionDataTable::functionDataTable() {
        this->TableName = L"function";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::functionDataTable::functionDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::functionDataTable::functionDataTable(::System::Runtime::Serialization::SerializationInfo^  info, ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionDataTable::mainExprColumn::get() {
        return this->columnmainExpr;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionDataTable::function_IdColumn::get() {
        return this->columnfunction_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionDataTable::classRange_IdColumn::get() {
        return this->columnclassRange_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionDataTable::parser_IdColumn::get() {
        return this->columnparser_Id;
    }
    
    inline ::System::Int32 NewDataSet::functionDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::functionDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::functionDataTable::AddfunctionRow(notepadPlusPlus::NewDataSet::functionRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::functionDataTable::AddfunctionRow(System::String^  mainExpr, 
                notepadPlusPlus::NewDataSet::classRangeRow^  parentclassRangeRowByclassRange_function, notepadPlusPlus::NewDataSet::parserRow^  parentparserRowByparser_function) {
        notepadPlusPlus::NewDataSet::functionRow^  rowfunctionRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(4) {mainExpr, nullptr, 
            nullptr, nullptr};
        if (parentclassRangeRowByclassRange_function != nullptr) {
            columnValuesArray[2] = parentclassRangeRowByclassRange_function[5];
        }
        if (parentparserRowByparser_function != nullptr) {
            columnValuesArray[3] = parentparserRowByparser_function[4];
        }
        rowfunctionRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowfunctionRow);
        return rowfunctionRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::functionDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionDataTable::Clone() {
        notepadPlusPlus::NewDataSet::functionDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::functionDataTable());
    }
    
    inline ::System::Void NewDataSet::functionDataTable::InitVars() {
        this->columnmainExpr = __super::Columns[L"mainExpr"];
        this->columnfunction_Id = __super::Columns[L"function_Id"];
        this->columnclassRange_Id = __super::Columns[L"classRange_Id"];
        this->columnparser_Id = __super::Columns[L"parser_Id"];
    }
    
    inline ::System::Void NewDataSet::functionDataTable::InitClass() {
        this->columnmainExpr = (gcnew ::System::Data::DataColumn(L"mainExpr", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnmainExpr);
        this->columnfunction_Id = (gcnew ::System::Data::DataColumn(L"function_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunction_Id);
        this->columnclassRange_Id = (gcnew ::System::Data::DataColumn(L"classRange_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnclassRange_Id);
        this->columnparser_Id = (gcnew ::System::Data::DataColumn(L"parser_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnparser_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnfunction_Id}, 
                true)));
        this->columnmainExpr->AllowDBNull = false;
        this->columnmainExpr->Namespace = L"";
        this->columnfunction_Id->AutoIncrement = true;
        this->columnfunction_Id->AllowDBNull = false;
        this->columnfunction_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::functionDataTable::NewfunctionRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::functionDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::functionRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::functionDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::functionRow::typeid;
    }
    
    inline ::System::Void NewDataSet::functionDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->functionRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::functionRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->functionRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::functionRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->functionRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::functionRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->functionRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::functionRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionDataTable::RemovefunctionRow(notepadPlusPlus::NewDataSet::functionRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::functionDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"functionDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::functionNameDataTable::functionNameDataTable() {
        this->TableName = L"functionName";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::functionNameDataTable::functionNameDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::functionNameDataTable::functionNameDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionNameDataTable::functionName_IdColumn::get() {
        return this->columnfunctionName_Id;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::functionNameDataTable::function_IdColumn::get() {
        return this->columnfunction_Id;
    }
    
    inline ::System::Int32 NewDataSet::functionNameDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::functionNameDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::AddfunctionNameRow(notepadPlusPlus::NewDataSet::functionNameRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::functionNameDataTable::AddfunctionNameRow(notepadPlusPlus::NewDataSet::functionRow^  parentfunctionRowByfunction_functionName) {
        notepadPlusPlus::NewDataSet::functionNameRow^  rowfunctionNameRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(2) {nullptr, nullptr};
        if (parentfunctionRowByfunction_functionName != nullptr) {
            columnValuesArray[1] = parentfunctionRowByfunction_functionName[1];
        }
        rowfunctionNameRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowfunctionNameRow);
        return rowfunctionNameRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::functionNameDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionNameDataTable::Clone() {
        notepadPlusPlus::NewDataSet::functionNameDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::functionNameDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::functionNameDataTable());
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::InitVars() {
        this->columnfunctionName_Id = __super::Columns[L"functionName_Id"];
        this->columnfunction_Id = __super::Columns[L"function_Id"];
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::InitClass() {
        this->columnfunctionName_Id = (gcnew ::System::Data::DataColumn(L"functionName_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionName_Id);
        this->columnfunction_Id = (gcnew ::System::Data::DataColumn(L"function_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunction_Id);
        this->Constraints->Add((gcnew ::System::Data::UniqueConstraint(L"Constraint1", gcnew cli::array< ::System::Data::DataColumn^  >(1) {this->columnfunctionName_Id}, 
                true)));
        this->columnfunctionName_Id->AutoIncrement = true;
        this->columnfunctionName_Id->AllowDBNull = false;
        this->columnfunctionName_Id->Unique = true;
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::functionNameDataTable::NewfunctionNameRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::functionNameDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::functionNameRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::functionNameDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::functionNameRow::typeid;
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->functionNameRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::functionNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->functionNameRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::functionNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->functionNameRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::functionNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->functionNameRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::functionNameRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::functionNameDataTable::RemovefunctionNameRow(notepadPlusPlus::NewDataSet::functionNameRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::functionNameDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"functionNameDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::funcNameExprDataTable::funcNameExprDataTable() {
        this->TableName = L"funcNameExpr";
        this->BeginInit();
        this->InitClass();
        this->EndInit();
    }
    
    inline NewDataSet::funcNameExprDataTable::funcNameExprDataTable(::System::Data::DataTable^  table) {
        this->TableName = table->TableName;
        if (table->CaseSensitive != table->DataSet->CaseSensitive) {
            this->CaseSensitive = table->CaseSensitive;
        }
        if (table->Locale->ToString() != table->DataSet->Locale->ToString()) {
            this->Locale = table->Locale;
        }
        if (table->Namespace != table->DataSet->Namespace) {
            this->Namespace = table->Namespace;
        }
        this->Prefix = table->Prefix;
        this->MinimumCapacity = table->MinimumCapacity;
    }
    
    inline NewDataSet::funcNameExprDataTable::funcNameExprDataTable(::System::Runtime::Serialization::SerializationInfo^  info, 
                ::System::Runtime::Serialization::StreamingContext context) : 
            ::System::Data::DataTable(info, context) {
        this->InitVars();
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::funcNameExprDataTable::exprColumn::get() {
        return this->columnexpr;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::funcNameExprDataTable::funcNameExpr_textColumn::get() {
        return this->columnfuncNameExpr_text;
    }
    
    inline ::System::Data::DataColumn^  NewDataSet::funcNameExprDataTable::functionName_IdColumn::get() {
        return this->columnfunctionName_Id;
    }
    
    inline ::System::Int32 NewDataSet::funcNameExprDataTable::Count::get() {
        return this->Rows->Count;
    }
    
    inline notepadPlusPlus::NewDataSet::funcNameExprRow^  NewDataSet::funcNameExprDataTable::default::get(::System::Int32 index) {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(this->Rows[index]));
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::AddfuncNameExprRow(notepadPlusPlus::NewDataSet::funcNameExprRow^  row) {
        this->Rows->Add(row);
    }
    
    inline notepadPlusPlus::NewDataSet::funcNameExprRow^  NewDataSet::funcNameExprDataTable::AddfuncNameExprRow(System::String^  expr, 
                System::String^  funcNameExpr_text, notepadPlusPlus::NewDataSet::functionNameRow^  parentfunctionNameRowByfunctionName_funcNameExpr) {
        notepadPlusPlus::NewDataSet::funcNameExprRow^  rowfuncNameExprRow = (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(this->NewRow()));
        cli::array< ::System::Object^  >^  columnValuesArray = gcnew cli::array< ::System::Object^  >(3) {expr, funcNameExpr_text, 
            nullptr};
        if (parentfunctionNameRowByfunctionName_funcNameExpr != nullptr) {
            columnValuesArray[2] = parentfunctionNameRowByfunctionName_funcNameExpr[0];
        }
        rowfuncNameExprRow->ItemArray = columnValuesArray;
        this->Rows->Add(rowfuncNameExprRow);
        return rowfuncNameExprRow;
    }
    
    inline ::System::Collections::IEnumerator^  NewDataSet::funcNameExprDataTable::GetEnumerator() {
        return this->Rows->GetEnumerator();
    }
    
    inline ::System::Data::DataTable^  NewDataSet::funcNameExprDataTable::Clone() {
        notepadPlusPlus::NewDataSet::funcNameExprDataTable^  cln = (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprDataTable^  >(__super::Clone()));
        cln->InitVars();
        return cln;
    }
    
    inline ::System::Data::DataTable^  NewDataSet::funcNameExprDataTable::CreateInstance() {
        return (gcnew notepadPlusPlus::NewDataSet::funcNameExprDataTable());
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::InitVars() {
        this->columnexpr = __super::Columns[L"expr"];
        this->columnfuncNameExpr_text = __super::Columns[L"funcNameExpr_text"];
        this->columnfunctionName_Id = __super::Columns[L"functionName_Id"];
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::InitClass() {
        this->columnexpr = (gcnew ::System::Data::DataColumn(L"expr", ::System::String::typeid, nullptr, ::System::Data::MappingType::Attribute));
        __super::Columns->Add(this->columnexpr);
        this->columnfuncNameExpr_text = (gcnew ::System::Data::DataColumn(L"funcNameExpr_text", ::System::String::typeid, nullptr, 
            ::System::Data::MappingType::SimpleContent));
        __super::Columns->Add(this->columnfuncNameExpr_text);
        this->columnfunctionName_Id = (gcnew ::System::Data::DataColumn(L"functionName_Id", ::System::Int32::typeid, nullptr, ::System::Data::MappingType::Hidden));
        __super::Columns->Add(this->columnfunctionName_Id);
        this->columnexpr->Namespace = L"";
        this->columnfuncNameExpr_text->AllowDBNull = false;
    }
    
    inline notepadPlusPlus::NewDataSet::funcNameExprRow^  NewDataSet::funcNameExprDataTable::NewfuncNameExprRow() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(this->NewRow()));
    }
    
    inline ::System::Data::DataRow^  NewDataSet::funcNameExprDataTable::NewRowFromBuilder(::System::Data::DataRowBuilder^  builder) {
        return (gcnew notepadPlusPlus::NewDataSet::funcNameExprRow(builder));
    }
    
    inline ::System::Type^  NewDataSet::funcNameExprDataTable::GetRowType() {
        return notepadPlusPlus::NewDataSet::funcNameExprRow::typeid;
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::OnRowChanged(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanged(e);
        {
            this->funcNameExprRowChanged(this, (gcnew notepadPlusPlus::NewDataSet::funcNameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::OnRowChanging(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowChanging(e);
        {
            this->funcNameExprRowChanging(this, (gcnew notepadPlusPlus::NewDataSet::funcNameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::OnRowDeleted(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleted(e);
        {
            this->funcNameExprRowDeleted(this, (gcnew notepadPlusPlus::NewDataSet::funcNameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::OnRowDeleting(::System::Data::DataRowChangeEventArgs^  e) {
        __super::OnRowDeleting(e);
        {
            this->funcNameExprRowDeleting(this, (gcnew notepadPlusPlus::NewDataSet::funcNameExprRowChangeEvent((cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprRow^  >(e->Row)), 
                    e->Action)));
        }
    }
    
    inline ::System::Void NewDataSet::funcNameExprDataTable::RemovefuncNameExprRow(notepadPlusPlus::NewDataSet::funcNameExprRow^  row) {
        this->Rows->Remove(row);
    }
    
    inline ::System::Xml::Schema::XmlSchemaComplexType^  NewDataSet::funcNameExprDataTable::GetTypedTableSchema(::System::Xml::Schema::XmlSchemaSet^  xs) {
        ::System::Xml::Schema::XmlSchemaComplexType^  type = (gcnew ::System::Xml::Schema::XmlSchemaComplexType());
        ::System::Xml::Schema::XmlSchemaSequence^  sequence = (gcnew ::System::Xml::Schema::XmlSchemaSequence());
        notepadPlusPlus::NewDataSet^  ds = (gcnew notepadPlusPlus::NewDataSet());
        ::System::Xml::Schema::XmlSchemaAny^  any1 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any1->Namespace = L"http://www.w3.org/2001/XMLSchema";
        any1->MinOccurs = ::System::Decimal(0);
        any1->MaxOccurs = ::System::Decimal::MaxValue;
        any1->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any1);
        ::System::Xml::Schema::XmlSchemaAny^  any2 = (gcnew ::System::Xml::Schema::XmlSchemaAny());
        any2->Namespace = L"urn:schemas-microsoft-com:xml-diffgram-v1";
        any2->MinOccurs = ::System::Decimal(1);
        any2->ProcessContents = ::System::Xml::Schema::XmlSchemaContentProcessing::Lax;
        sequence->Items->Add(any2);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute1 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute1->Name = L"namespace";
        attribute1->FixedValue = ds->Namespace;
        type->Attributes->Add(attribute1);
        ::System::Xml::Schema::XmlSchemaAttribute^  attribute2 = (gcnew ::System::Xml::Schema::XmlSchemaAttribute());
        attribute2->Name = L"tableTypeName";
        attribute2->FixedValue = L"funcNameExprDataTable";
        type->Attributes->Add(attribute2);
        type->Particle = sequence;
        ::System::Xml::Schema::XmlSchema^  dsSchema = ds->GetSchemaSerializable();
        if (xs->Contains(dsSchema->TargetNamespace)) {
            ::System::IO::MemoryStream^  s1 = (gcnew ::System::IO::MemoryStream());
            ::System::IO::MemoryStream^  s2 = (gcnew ::System::IO::MemoryStream());
            try {
                ::System::Xml::Schema::XmlSchema^  schema = nullptr;
                dsSchema->Write(s1);
                for (                ::System::Collections::IEnumerator^  schemas = xs->Schemas(dsSchema->TargetNamespace)->GetEnumerator(); schemas->MoveNext();                 ) {
                    schema = (cli::safe_cast<::System::Xml::Schema::XmlSchema^  >(schemas->Current));
                    s2->SetLength(0);
                    schema->Write(s2);
                    if (s1->Length == s2->Length) {
                        s1->Position = 0;
                        s2->Position = 0;
                        for (                        ; ((s1->Position != s1->Length) 
                                    && (s1->ReadByte() == s2->ReadByte()));                         ) {
                            ;
                        }
                        if (s1->Position == s1->Length) {
                            return type;
                        }
                    }
                }
            }
            finally {
                if (s1 != nullptr) {
                    s1->Close();
                }
                if (s2 != nullptr) {
                    s2->Close();
                }
            }
        }
        xs->Add(dsSchema);
        return type;
    }
    
    
    inline NewDataSet::NotepadPlusRow::NotepadPlusRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableNotepadPlus = (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::NotepadPlusRow::NotepadPlus_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableNotepadPlus->NotepadPlus_IdColumn]));
    }
    inline System::Void NewDataSet::NotepadPlusRow::NotepadPlus_Id::set(System::Int32 value) {
        this[this->tableNotepadPlus->NotepadPlus_IdColumn] = value;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::functionListRow^  >^  NewDataSet::NotepadPlusRow::GetfunctionListRows() {
        if (this->Table->ChildRelations[L"NotepadPlus_functionList"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::functionListRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::functionListRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"NotepadPlus_functionList"])));
        }
    }
    
    
    inline NewDataSet::functionListRow::functionListRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablefunctionList = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::functionListRow::functionList_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tablefunctionList->functionList_IdColumn]));
    }
    inline System::Void NewDataSet::functionListRow::functionList_Id::set(System::Int32 value) {
        this[this->tablefunctionList->functionList_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::functionListRow::NotepadPlus_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablefunctionList->NotepadPlus_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'NotepadPlus_Id\' в таблице \'functionList\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::functionListRow::NotepadPlus_Id::set(System::Int32 value) {
        this[this->tablefunctionList->NotepadPlus_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewDataSet::functionListRow::NotepadPlusRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::NotepadPlusRow^  >(this->GetParentRow(this->Table->ParentRelations[L"NotepadPlus_functionList"])));
    }
    inline System::Void NewDataSet::functionListRow::NotepadPlusRow::set(notepadPlusPlus::NewDataSet::NotepadPlusRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"NotepadPlus_functionList"]);
    }
    
    inline ::System::Boolean NewDataSet::functionListRow::IsNotepadPlus_IdNull() {
        return this->IsNull(this->tablefunctionList->NotepadPlus_IdColumn);
    }
    
    inline ::System::Void NewDataSet::functionListRow::SetNotepadPlus_IdNull() {
        this[this->tablefunctionList->NotepadPlus_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::associationMapRow^  >^  NewDataSet::functionListRow::GetassociationMapRows() {
        if (this->Table->ChildRelations[L"functionList_associationMap"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::associationMapRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::associationMapRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"functionList_associationMap"])));
        }
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::parsersRow^  >^  NewDataSet::functionListRow::GetparsersRows() {
        if (this->Table->ChildRelations[L"functionList_parsers"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::parsersRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::parsersRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"functionList_parsers"])));
        }
    }
    
    
    inline NewDataSet::associationMapRow::associationMapRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableassociationMap = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::associationMapRow::associationMap_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableassociationMap->associationMap_IdColumn]));
    }
    inline System::Void NewDataSet::associationMapRow::associationMap_Id::set(System::Int32 value) {
        this[this->tableassociationMap->associationMap_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::associationMapRow::functionList_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableassociationMap->functionList_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'functionList_Id\' в таблице \'associationMap\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::associationMapRow::functionList_Id::set(System::Int32 value) {
        this[this->tableassociationMap->functionList_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::associationMapRow::functionListRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(this->GetParentRow(this->Table->ParentRelations[L"functionList_associationMap"])));
    }
    inline System::Void NewDataSet::associationMapRow::functionListRow::set(notepadPlusPlus::NewDataSet::functionListRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"functionList_associationMap"]);
    }
    
    inline ::System::Boolean NewDataSet::associationMapRow::IsfunctionList_IdNull() {
        return this->IsNull(this->tableassociationMap->functionList_IdColumn);
    }
    
    inline ::System::Void NewDataSet::associationMapRow::SetfunctionList_IdNull() {
        this[this->tableassociationMap->functionList_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::associationRow^  >^  NewDataSet::associationMapRow::GetassociationRows() {
        if (this->Table->ChildRelations[L"associationMap_association"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::associationRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::associationRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"associationMap_association"])));
        }
    }
    
    
    inline NewDataSet::associationRow::associationRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableassociation = (cli::safe_cast<notepadPlusPlus::NewDataSet::associationDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::associationRow::id::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableassociation->idColumn]));
    }
    inline System::Void NewDataSet::associationRow::id::set(System::String^  value) {
        this[this->tableassociation->idColumn] = value;
    }
    
    inline System::SByte NewDataSet::associationRow::langID::get() {
        try {
            return (cli::safe_cast<::System::SByte >(this[this->tableassociation->langIDColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'langID\' в таблице \'association\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::associationRow::langID::set(System::SByte value) {
        this[this->tableassociation->langIDColumn] = value;
    }
    
    inline System::String^  NewDataSet::associationRow::userDefinedLangName::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableassociation->userDefinedLangNameColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'userDefinedLangName\' в таблице \'association\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::associationRow::userDefinedLangName::set(System::String^  value) {
        this[this->tableassociation->userDefinedLangNameColumn] = value;
    }
    
    inline System::String^  NewDataSet::associationRow::ext::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableassociation->extColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'ext\' в таблице \'association\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::associationRow::ext::set(System::String^  value) {
        this[this->tableassociation->extColumn] = value;
    }
    
    inline System::String^  NewDataSet::associationRow::association_text::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableassociation->association_textColumn]));
    }
    inline System::Void NewDataSet::associationRow::association_text::set(System::String^  value) {
        this[this->tableassociation->association_textColumn] = value;
    }
    
    inline System::Int32 NewDataSet::associationRow::associationMap_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableassociation->associationMap_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'associationMap_Id\' в таблице \'association\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::associationRow::associationMap_Id::set(System::Int32 value) {
        this[this->tableassociation->associationMap_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapRow^  NewDataSet::associationRow::associationMapRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::associationMapRow^  >(this->GetParentRow(this->Table->ParentRelations[L"associationMap_association"])));
    }
    inline System::Void NewDataSet::associationRow::associationMapRow::set(notepadPlusPlus::NewDataSet::associationMapRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"associationMap_association"]);
    }
    
    inline ::System::Boolean NewDataSet::associationRow::IslangIDNull() {
        return this->IsNull(this->tableassociation->langIDColumn);
    }
    
    inline ::System::Void NewDataSet::associationRow::SetlangIDNull() {
        this[this->tableassociation->langIDColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::associationRow::IsuserDefinedLangNameNull() {
        return this->IsNull(this->tableassociation->userDefinedLangNameColumn);
    }
    
    inline ::System::Void NewDataSet::associationRow::SetuserDefinedLangNameNull() {
        this[this->tableassociation->userDefinedLangNameColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::associationRow::IsextNull() {
        return this->IsNull(this->tableassociation->extColumn);
    }
    
    inline ::System::Void NewDataSet::associationRow::SetextNull() {
        this[this->tableassociation->extColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::associationRow::IsassociationMap_IdNull() {
        return this->IsNull(this->tableassociation->associationMap_IdColumn);
    }
    
    inline ::System::Void NewDataSet::associationRow::SetassociationMap_IdNull() {
        this[this->tableassociation->associationMap_IdColumn] = ::System::Convert::DBNull;
    }
    
    
    inline NewDataSet::parsersRow::parsersRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableparsers = (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::parsersRow::parsers_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableparsers->parsers_IdColumn]));
    }
    inline System::Void NewDataSet::parsersRow::parsers_Id::set(System::Int32 value) {
        this[this->tableparsers->parsers_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::parsersRow::functionList_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableparsers->functionList_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'functionList_Id\' в таблице \'parsers\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::parsersRow::functionList_Id::set(System::Int32 value) {
        this[this->tableparsers->functionList_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::parsersRow::functionListRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionListRow^  >(this->GetParentRow(this->Table->ParentRelations[L"functionList_parsers"])));
    }
    inline System::Void NewDataSet::parsersRow::functionListRow::set(notepadPlusPlus::NewDataSet::functionListRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"functionList_parsers"]);
    }
    
    inline ::System::Boolean NewDataSet::parsersRow::IsfunctionList_IdNull() {
        return this->IsNull(this->tableparsers->functionList_IdColumn);
    }
    
    inline ::System::Void NewDataSet::parsersRow::SetfunctionList_IdNull() {
        this[this->tableparsers->functionList_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::parserRow^  >^  NewDataSet::parsersRow::GetparserRows() {
        if (this->Table->ChildRelations[L"parsers_parser"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::parserRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::parserRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"parsers_parser"])));
        }
    }
    
    
    inline NewDataSet::parserRow::parserRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableparser = (cli::safe_cast<notepadPlusPlus::NewDataSet::parserDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::parserRow::id::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableparser->idColumn]));
    }
    inline System::Void NewDataSet::parserRow::id::set(System::String^  value) {
        this[this->tableparser->idColumn] = value;
    }
    
    inline System::String^  NewDataSet::parserRow::displayName::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableparser->displayNameColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'displayName\' в таблице \'parser\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::parserRow::displayName::set(System::String^  value) {
        this[this->tableparser->displayNameColumn] = value;
    }
    
    inline System::String^  NewDataSet::parserRow::commentExpr::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableparser->commentExprColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'commentExpr\' в таблице \'parser\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::parserRow::commentExpr::set(System::String^  value) {
        this[this->tableparser->commentExprColumn] = value;
    }
    
    inline System::String^  NewDataSet::parserRow::version::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableparser->versionColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'version\' в таблице \'parser\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::parserRow::version::set(System::String^  value) {
        this[this->tableparser->versionColumn] = value;
    }
    
    inline System::Int32 NewDataSet::parserRow::parser_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableparser->parser_IdColumn]));
    }
    inline System::Void NewDataSet::parserRow::parser_Id::set(System::Int32 value) {
        this[this->tableparser->parser_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::parserRow::parsers_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableparser->parsers_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'parsers_Id\' в таблице \'parser\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::parserRow::parsers_Id::set(System::Int32 value) {
        this[this->tableparser->parsers_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::parsersRow^  NewDataSet::parserRow::parsersRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parsersRow^  >(this->GetParentRow(this->Table->ParentRelations[L"parsers_parser"])));
    }
    inline System::Void NewDataSet::parserRow::parsersRow::set(notepadPlusPlus::NewDataSet::parsersRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"parsers_parser"]);
    }
    
    inline ::System::Boolean NewDataSet::parserRow::IsdisplayNameNull() {
        return this->IsNull(this->tableparser->displayNameColumn);
    }
    
    inline ::System::Void NewDataSet::parserRow::SetdisplayNameNull() {
        this[this->tableparser->displayNameColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::parserRow::IscommentExprNull() {
        return this->IsNull(this->tableparser->commentExprColumn);
    }
    
    inline ::System::Void NewDataSet::parserRow::SetcommentExprNull() {
        this[this->tableparser->commentExprColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::parserRow::IsversionNull() {
        return this->IsNull(this->tableparser->versionColumn);
    }
    
    inline ::System::Void NewDataSet::parserRow::SetversionNull() {
        this[this->tableparser->versionColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::parserRow::Isparsers_IdNull() {
        return this->IsNull(this->tableparser->parsers_IdColumn);
    }
    
    inline ::System::Void NewDataSet::parserRow::Setparsers_IdNull() {
        this[this->tableparser->parsers_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::classRangeRow^  >^  NewDataSet::parserRow::GetclassRangeRows() {
        if (this->Table->ChildRelations[L"parser_classRange"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::classRangeRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::classRangeRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"parser_classRange"])));
        }
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  NewDataSet::parserRow::GetfunctionRows() {
        if (this->Table->ChildRelations[L"parser_function"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::functionRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"parser_function"])));
        }
    }
    
    
    inline NewDataSet::classRangeRow::classRangeRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableclassRange = (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::classRangeRow::mainExpr::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tableclassRange->mainExprColumn]));
    }
    inline System::Void NewDataSet::classRangeRow::mainExpr::set(System::String^  value) {
        this[this->tableclassRange->mainExprColumn] = value;
    }
    
    inline System::String^  NewDataSet::classRangeRow::openSymbole::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableclassRange->openSymboleColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'openSymbole\' в таблице \'classRange\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classRangeRow::openSymbole::set(System::String^  value) {
        this[this->tableclassRange->openSymboleColumn] = value;
    }
    
    inline System::String^  NewDataSet::classRangeRow::closeSymbole::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableclassRange->closeSymboleColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'closeSymbole\' в таблице \'classRange\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classRangeRow::closeSymbole::set(System::String^  value) {
        this[this->tableclassRange->closeSymboleColumn] = value;
    }
    
    inline System::String^  NewDataSet::classRangeRow::openSymbol::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableclassRange->openSymbolColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'openSymbol\' в таблице \'classRange\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classRangeRow::openSymbol::set(System::String^  value) {
        this[this->tableclassRange->openSymbolColumn] = value;
    }
    
    inline System::String^  NewDataSet::classRangeRow::closeSymbol::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tableclassRange->closeSymbolColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'closeSymbol\' в таблице \'classRange\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classRangeRow::closeSymbol::set(System::String^  value) {
        this[this->tableclassRange->closeSymbolColumn] = value;
    }
    
    inline System::Int32 NewDataSet::classRangeRow::classRange_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableclassRange->classRange_IdColumn]));
    }
    inline System::Void NewDataSet::classRangeRow::classRange_Id::set(System::Int32 value) {
        this[this->tableclassRange->classRange_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::classRangeRow::parser_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableclassRange->parser_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'parser_Id\' в таблице \'classRange\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classRangeRow::parser_Id::set(System::Int32 value) {
        this[this->tableclassRange->parser_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::classRangeRow::parserRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(this->GetParentRow(this->Table->ParentRelations[L"parser_classRange"])));
    }
    inline System::Void NewDataSet::classRangeRow::parserRow::set(notepadPlusPlus::NewDataSet::parserRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"parser_classRange"]);
    }
    
    inline ::System::Boolean NewDataSet::classRangeRow::IsopenSymboleNull() {
        return this->IsNull(this->tableclassRange->openSymboleColumn);
    }
    
    inline ::System::Void NewDataSet::classRangeRow::SetopenSymboleNull() {
        this[this->tableclassRange->openSymboleColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::classRangeRow::IscloseSymboleNull() {
        return this->IsNull(this->tableclassRange->closeSymboleColumn);
    }
    
    inline ::System::Void NewDataSet::classRangeRow::SetcloseSymboleNull() {
        this[this->tableclassRange->closeSymboleColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::classRangeRow::IsopenSymbolNull() {
        return this->IsNull(this->tableclassRange->openSymbolColumn);
    }
    
    inline ::System::Void NewDataSet::classRangeRow::SetopenSymbolNull() {
        this[this->tableclassRange->openSymbolColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::classRangeRow::IscloseSymbolNull() {
        return this->IsNull(this->tableclassRange->closeSymbolColumn);
    }
    
    inline ::System::Void NewDataSet::classRangeRow::SetcloseSymbolNull() {
        this[this->tableclassRange->closeSymbolColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::classRangeRow::Isparser_IdNull() {
        return this->IsNull(this->tableclassRange->parser_IdColumn);
    }
    
    inline ::System::Void NewDataSet::classRangeRow::Setparser_IdNull() {
        this[this->tableclassRange->parser_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  NewDataSet::classRangeRow::GetclassNameRows() {
        if (this->Table->ChildRelations[L"classRange_className"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"classRange_className"])));
        }
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  NewDataSet::classRangeRow::GetfunctionRows() {
        if (this->Table->ChildRelations[L"classRange_function"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::functionRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::functionRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"classRange_function"])));
        }
    }
    
    
    inline NewDataSet::classNameRow::classNameRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tableclassName = (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::classNameRow::className_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tableclassName->className_IdColumn]));
    }
    inline System::Void NewDataSet::classNameRow::className_Id::set(System::Int32 value) {
        this[this->tableclassName->className_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::classNameRow::function_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableclassName->function_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'function_Id\' в таблице \'className\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classNameRow::function_Id::set(System::Int32 value) {
        this[this->tableclassName->function_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::classNameRow::classRange_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tableclassName->classRange_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'classRange_Id\' в таблице \'className\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::classNameRow::classRange_Id::set(System::Int32 value) {
        this[this->tableclassName->classRange_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::classNameRow::functionRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(this->GetParentRow(this->Table->ParentRelations[L"function_className"])));
    }
    inline System::Void NewDataSet::classNameRow::functionRow::set(notepadPlusPlus::NewDataSet::functionRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"function_className"]);
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::classNameRow::classRangeRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(this->GetParentRow(this->Table->ParentRelations[L"classRange_className"])));
    }
    inline System::Void NewDataSet::classNameRow::classRangeRow::set(notepadPlusPlus::NewDataSet::classRangeRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"classRange_className"]);
    }
    
    inline ::System::Boolean NewDataSet::classNameRow::Isfunction_IdNull() {
        return this->IsNull(this->tableclassName->function_IdColumn);
    }
    
    inline ::System::Void NewDataSet::classNameRow::Setfunction_IdNull() {
        this[this->tableclassName->function_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::classNameRow::IsclassRange_IdNull() {
        return this->IsNull(this->tableclassName->classRange_IdColumn);
    }
    
    inline ::System::Void NewDataSet::classNameRow::SetclassRange_IdNull() {
        this[this->tableclassName->classRange_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  NewDataSet::classNameRow::GetnameExprRows() {
        if (this->Table->ChildRelations[L"className_nameExpr"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"className_nameExpr"])));
        }
    }
    
    
    inline NewDataSet::nameExprRow::nameExprRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablenameExpr = (cli::safe_cast<notepadPlusPlus::NewDataSet::nameExprDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::nameExprRow::expr::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tablenameExpr->exprColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'expr\' в таблице \'nameExpr\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::nameExprRow::expr::set(System::String^  value) {
        this[this->tablenameExpr->exprColumn] = value;
    }
    
    inline System::String^  NewDataSet::nameExprRow::nameExpr_text::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tablenameExpr->nameExpr_textColumn]));
    }
    inline System::Void NewDataSet::nameExprRow::nameExpr_text::set(System::String^  value) {
        this[this->tablenameExpr->nameExpr_textColumn] = value;
    }
    
    inline System::Int32 NewDataSet::nameExprRow::className_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablenameExpr->className_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'className_Id\' в таблице \'nameExpr\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::nameExprRow::className_Id::set(System::Int32 value) {
        this[this->tablenameExpr->className_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::nameExprRow::functionName_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablenameExpr->functionName_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'functionName_Id\' в таблице \'nameExpr\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::nameExprRow::functionName_Id::set(System::Int32 value) {
        this[this->tablenameExpr->functionName_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::classNameRow^  NewDataSet::nameExprRow::classNameRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classNameRow^  >(this->GetParentRow(this->Table->ParentRelations[L"className_nameExpr"])));
    }
    inline System::Void NewDataSet::nameExprRow::classNameRow::set(notepadPlusPlus::NewDataSet::classNameRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"className_nameExpr"]);
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::nameExprRow::functionNameRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(this->GetParentRow(this->Table->ParentRelations[L"functionName_nameExpr"])));
    }
    inline System::Void NewDataSet::nameExprRow::functionNameRow::set(notepadPlusPlus::NewDataSet::functionNameRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"functionName_nameExpr"]);
    }
    
    inline ::System::Boolean NewDataSet::nameExprRow::IsexprNull() {
        return this->IsNull(this->tablenameExpr->exprColumn);
    }
    
    inline ::System::Void NewDataSet::nameExprRow::SetexprNull() {
        this[this->tablenameExpr->exprColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::nameExprRow::IsclassName_IdNull() {
        return this->IsNull(this->tablenameExpr->className_IdColumn);
    }
    
    inline ::System::Void NewDataSet::nameExprRow::SetclassName_IdNull() {
        this[this->tablenameExpr->className_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::nameExprRow::IsfunctionName_IdNull() {
        return this->IsNull(this->tablenameExpr->functionName_IdColumn);
    }
    
    inline ::System::Void NewDataSet::nameExprRow::SetfunctionName_IdNull() {
        this[this->tablenameExpr->functionName_IdColumn] = ::System::Convert::DBNull;
    }
    
    
    inline NewDataSet::functionRow::functionRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablefunction = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::functionRow::mainExpr::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tablefunction->mainExprColumn]));
    }
    inline System::Void NewDataSet::functionRow::mainExpr::set(System::String^  value) {
        this[this->tablefunction->mainExprColumn] = value;
    }
    
    inline System::Int32 NewDataSet::functionRow::function_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tablefunction->function_IdColumn]));
    }
    inline System::Void NewDataSet::functionRow::function_Id::set(System::Int32 value) {
        this[this->tablefunction->function_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::functionRow::classRange_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablefunction->classRange_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'classRange_Id\' в таблице \'function\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::functionRow::classRange_Id::set(System::Int32 value) {
        this[this->tablefunction->classRange_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::functionRow::parser_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablefunction->parser_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'parser_Id\' в таблице \'function\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::functionRow::parser_Id::set(System::Int32 value) {
        this[this->tablefunction->parser_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::functionRow::classRangeRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::classRangeRow^  >(this->GetParentRow(this->Table->ParentRelations[L"classRange_function"])));
    }
    inline System::Void NewDataSet::functionRow::classRangeRow::set(notepadPlusPlus::NewDataSet::classRangeRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"classRange_function"]);
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::functionRow::parserRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::parserRow^  >(this->GetParentRow(this->Table->ParentRelations[L"parser_function"])));
    }
    inline System::Void NewDataSet::functionRow::parserRow::set(notepadPlusPlus::NewDataSet::parserRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"parser_function"]);
    }
    
    inline ::System::Boolean NewDataSet::functionRow::IsclassRange_IdNull() {
        return this->IsNull(this->tablefunction->classRange_IdColumn);
    }
    
    inline ::System::Void NewDataSet::functionRow::SetclassRange_IdNull() {
        this[this->tablefunction->classRange_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::functionRow::Isparser_IdNull() {
        return this->IsNull(this->tablefunction->parser_IdColumn);
    }
    
    inline ::System::Void NewDataSet::functionRow::Setparser_IdNull() {
        this[this->tablefunction->parser_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::functionNameRow^  >^  NewDataSet::functionRow::GetfunctionNameRows() {
        if (this->Table->ChildRelations[L"function_functionName"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::functionNameRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::functionNameRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"function_functionName"])));
        }
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  NewDataSet::functionRow::GetclassNameRows() {
        if (this->Table->ChildRelations[L"function_className"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::classNameRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"function_className"])));
        }
    }
    
    
    inline NewDataSet::functionNameRow::functionNameRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablefunctionName = (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameDataTable^  >(this->Table));
    }
    
    inline System::Int32 NewDataSet::functionNameRow::functionName_Id::get() {
        return (cli::safe_cast<::System::Int32 >(this[this->tablefunctionName->functionName_IdColumn]));
    }
    inline System::Void NewDataSet::functionNameRow::functionName_Id::set(System::Int32 value) {
        this[this->tablefunctionName->functionName_IdColumn] = value;
    }
    
    inline System::Int32 NewDataSet::functionNameRow::function_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablefunctionName->function_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'function_Id\' в таблице \'functionName\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::functionNameRow::function_Id::set(System::Int32 value) {
        this[this->tablefunctionName->function_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::functionNameRow::functionRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionRow^  >(this->GetParentRow(this->Table->ParentRelations[L"function_functionName"])));
    }
    inline System::Void NewDataSet::functionNameRow::functionRow::set(notepadPlusPlus::NewDataSet::functionRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"function_functionName"]);
    }
    
    inline ::System::Boolean NewDataSet::functionNameRow::Isfunction_IdNull() {
        return this->IsNull(this->tablefunctionName->function_IdColumn);
    }
    
    inline ::System::Void NewDataSet::functionNameRow::Setfunction_IdNull() {
        this[this->tablefunctionName->function_IdColumn] = ::System::Convert::DBNull;
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::funcNameExprRow^  >^  NewDataSet::functionNameRow::GetfuncNameExprRows() {
        if (this->Table->ChildRelations[L"functionName_funcNameExpr"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::funcNameExprRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::funcNameExprRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"functionName_funcNameExpr"])));
        }
    }
    
    inline cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  NewDataSet::functionNameRow::GetnameExprRows() {
        if (this->Table->ChildRelations[L"functionName_nameExpr"] == nullptr) {
            return gcnew cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >(0);
        }
        else {
            return (cli::safe_cast<cli::array< notepadPlusPlus::NewDataSet::nameExprRow^  >^  >(__super::GetChildRows(this->Table->ChildRelations[L"functionName_nameExpr"])));
        }
    }
    
    
    inline NewDataSet::funcNameExprRow::funcNameExprRow(::System::Data::DataRowBuilder^  rb) : 
            ::System::Data::DataRow(rb) {
        this->tablefuncNameExpr = (cli::safe_cast<notepadPlusPlus::NewDataSet::funcNameExprDataTable^  >(this->Table));
    }
    
    inline System::String^  NewDataSet::funcNameExprRow::expr::get() {
        try {
            return (cli::safe_cast<::System::String^  >(this[this->tablefuncNameExpr->exprColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'expr\' в таблице \'funcNameExpr\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::funcNameExprRow::expr::set(System::String^  value) {
        this[this->tablefuncNameExpr->exprColumn] = value;
    }
    
    inline System::String^  NewDataSet::funcNameExprRow::funcNameExpr_text::get() {
        return (cli::safe_cast<::System::String^  >(this[this->tablefuncNameExpr->funcNameExpr_textColumn]));
    }
    inline System::Void NewDataSet::funcNameExprRow::funcNameExpr_text::set(System::String^  value) {
        this[this->tablefuncNameExpr->funcNameExpr_textColumn] = value;
    }
    
    inline System::Int32 NewDataSet::funcNameExprRow::functionName_Id::get() {
        try {
            return (cli::safe_cast<::System::Int32 >(this[this->tablefuncNameExpr->functionName_IdColumn]));
        }
        catch (::System::InvalidCastException^ e) {
            throw (gcnew ::System::Data::StrongTypingException(L"Значение для столбца \'functionName_Id\' в таблице \'funcNameExpr\' равно DBNull.", 
                e));
        }
    }
    inline System::Void NewDataSet::funcNameExprRow::functionName_Id::set(System::Int32 value) {
        this[this->tablefuncNameExpr->functionName_IdColumn] = value;
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::funcNameExprRow::functionNameRow::get() {
        return (cli::safe_cast<notepadPlusPlus::NewDataSet::functionNameRow^  >(this->GetParentRow(this->Table->ParentRelations[L"functionName_funcNameExpr"])));
    }
    inline System::Void NewDataSet::funcNameExprRow::functionNameRow::set(notepadPlusPlus::NewDataSet::functionNameRow^  value) {
        this->SetParentRow(value, this->Table->ParentRelations[L"functionName_funcNameExpr"]);
    }
    
    inline ::System::Boolean NewDataSet::funcNameExprRow::IsexprNull() {
        return this->IsNull(this->tablefuncNameExpr->exprColumn);
    }
    
    inline ::System::Void NewDataSet::funcNameExprRow::SetexprNull() {
        this[this->tablefuncNameExpr->exprColumn] = ::System::Convert::DBNull;
    }
    
    inline ::System::Boolean NewDataSet::funcNameExprRow::IsfunctionName_IdNull() {
        return this->IsNull(this->tablefuncNameExpr->functionName_IdColumn);
    }
    
    inline ::System::Void NewDataSet::funcNameExprRow::SetfunctionName_IdNull() {
        this[this->tablefuncNameExpr->functionName_IdColumn] = ::System::Convert::DBNull;
    }
    
    
    inline NewDataSet::NotepadPlusRowChangeEvent::NotepadPlusRowChangeEvent(notepadPlusPlus::NewDataSet::NotepadPlusRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::NotepadPlusRow^  NewDataSet::NotepadPlusRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::NotepadPlusRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::functionListRowChangeEvent::functionListRowChangeEvent(notepadPlusPlus::NewDataSet::functionListRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::functionListRow^  NewDataSet::functionListRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::functionListRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::associationMapRowChangeEvent::associationMapRowChangeEvent(notepadPlusPlus::NewDataSet::associationMapRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::associationMapRow^  NewDataSet::associationMapRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::associationMapRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::associationRowChangeEvent::associationRowChangeEvent(notepadPlusPlus::NewDataSet::associationRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::associationRow^  NewDataSet::associationRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::associationRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::parsersRowChangeEvent::parsersRowChangeEvent(notepadPlusPlus::NewDataSet::parsersRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::parsersRow^  NewDataSet::parsersRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::parsersRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::parserRowChangeEvent::parserRowChangeEvent(notepadPlusPlus::NewDataSet::parserRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::parserRow^  NewDataSet::parserRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::parserRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::classRangeRowChangeEvent::classRangeRowChangeEvent(notepadPlusPlus::NewDataSet::classRangeRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::classRangeRow^  NewDataSet::classRangeRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::classRangeRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::classNameRowChangeEvent::classNameRowChangeEvent(notepadPlusPlus::NewDataSet::classNameRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::classNameRow^  NewDataSet::classNameRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::classNameRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::nameExprRowChangeEvent::nameExprRowChangeEvent(notepadPlusPlus::NewDataSet::nameExprRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::nameExprRow^  NewDataSet::nameExprRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::nameExprRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::functionRowChangeEvent::functionRowChangeEvent(notepadPlusPlus::NewDataSet::functionRow^  row, ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::functionRow^  NewDataSet::functionRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::functionRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::functionNameRowChangeEvent::functionNameRowChangeEvent(notepadPlusPlus::NewDataSet::functionNameRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::functionNameRow^  NewDataSet::functionNameRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::functionNameRowChangeEvent::Action::get() {
        return this->eventAction;
    }
    
    
    inline NewDataSet::funcNameExprRowChangeEvent::funcNameExprRowChangeEvent(notepadPlusPlus::NewDataSet::funcNameExprRow^  row, 
                ::System::Data::DataRowAction action) {
        this->eventRow = row;
        this->eventAction = action;
    }
    
    inline notepadPlusPlus::NewDataSet::funcNameExprRow^  NewDataSet::funcNameExprRowChangeEvent::Row::get() {
        return this->eventRow;
    }
    
    inline ::System::Data::DataRowAction NewDataSet::funcNameExprRowChangeEvent::Action::get() {
        return this->eventAction;
    }
}
