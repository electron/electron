<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>SOAA - Sistema de Registro</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css" rel="stylesheet">
  <style>
    /* Custom scrollbar for better aesthetics */
    ::-webkit-scrollbar {
      width: 8px;
      height: 8px;
    }
    ::-webkit-scrollbar-track {
      background: #f1f1f1;
      border-radius: 10px;
    }
    ::-webkit-scrollbar-thumb {
      background: #888;
      border-radius: 10px;
    }
    ::-webkit-scrollbar-thumb:hover {
      background: #555;
    }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    }
    /* Ensure select options in table have visible text against dynamic backgrounds */
    .status-select option {
        background-color: white !important; /* Force background for options */
        color: black !important; /* Force text color for options */
    }
    .custom-scrollbar::-webkit-scrollbar {
      width: 6px;
      height: 6px;
    }
    .custom-scrollbar::-webkit-scrollbar-thumb {
      background-color: #a0aec0; /* gray-500 */
      border-radius: 10px;
    }
    .custom-scrollbar::-webkit-scrollbar-track {
      background-color: #edf2f7; /* gray-200 */
      border-radius: 10px;
    }
  </style>
<script type="importmap">
{
  "imports": {
    "react-dom/": "https://esm.sh/react-dom@^19.1.0/",
    "react/": "https://esm.sh/react@^19.1.0/",
    "react": "https://esm.sh/react@^19.1.0"
  }
}
</script>
</head>
<body class="bg-gray-100">
  <div id="root"></div>
  <script type="module">
    import React, { useState, useCallback, useMemo, useEffect, useRef } from 'react';
    import ReactDOM from 'react-dom/client';

    // --- Content from types.ts (converted to JS) ---
    const OperationType = {
      IMPORTACION: "Importación",
      EXPORTACION: "Exportación",
      VIRTUAL: "Virtual",
    };

    const MainStatus = {
      REVISION: "REVISION",
      ELABORACION: "ELABORACION",
      MODULO: "MODULO",
      RECONOCIMIENTO_LIBERADO: "RECONOCIMIENTO LIBERADO",
      JURIDICO: "JURIDICO",
      ENTREGA_CONCLUIDA: "ENTREGA CONCLUIDA",
      RECONOCIMIENTO_ADUANERO: "RECONOCIMIENTO ADUANERO",
      GLOSA: "GLOSA",
      DOCUMENTACION: "DOCUMENTACION",
      DESADUANAMIENTO_LIBRE: "DESADUANAMIENTO LIBRE",
      SUPERVISION: "SUPERVISION",
      VENTANILLA: "VENTANILLA",
      EN_ESPERA_DE_CP: "EN ESPERA DE CP",
      PEDIMENTO_LISTO: "PEDIMENTO LISTO",
      PEDIMENTO_ENTREGADO: "PEDIMENTO ENTREGADO",
      COMBINADA: "COMBINADA",
      TRAFICO: "TRAFICO",
    };

    // --- Content from constants.ts ---
    const STATUS_CONFIG = [
      { id: MainStatus.REVISION, label: "REVISION", color: "bg-orange-500", group: 1 },
      { id: MainStatus.ELABORACION, label: "ELABORACION", color: "bg-blue-700", group: 1 },
      { id: MainStatus.MODULO, label: "MODULO", color: "bg-teal-500", group: 1 },
      { id: MainStatus.RECONOCIMIENTO_LIBERADO, label: "RECONOCIMIENTO LIBERADO", color: "bg-lime-500", textColor: "text-black", group: 2 },
      { id: MainStatus.JURIDICO, label: "JURIDICO", color: "bg-indigo-500", group: 2 },
      { id: MainStatus.ENTREGA_CONCLUIDA, label: "ENTREGA CONCLUIDA", color: "bg-green-600", group: 2 },
      { id: MainStatus.RECONOCIMIENTO_ADUANERO, label: "RECONOCIMIENTO ADUANERO", color: "bg-red-500", group: 3 },
      { id: MainStatus.GLOSA, label: "GLOSA", color: "bg-fuchsia-400", group: 3 },
      { id: MainStatus.DOCUMENTACION, label: "DOCUMENTACION", color: "bg-yellow-300", textColor: "text-black", group: 3 },
      { id: MainStatus.DESADUANAMIENTO_LIBRE, label: "DESADUANAMIENTO LIBRE", color: "bg-emerald-500", group: 4 },
      { id: MainStatus.SUPERVISION, label: "SUPERVISION", color: "bg-rose-400", group: 4 },
      { id: MainStatus.VENTANILLA, label: "VENTANILLA", color: "bg-amber-300", textColor: "text-black", group: 4 },
      { id: MainStatus.EN_ESPERA_DE_CP, label: "EN ESPERA DE CP", color: "bg-cyan-500", group: 5 },
      { id: MainStatus.PEDIMENTO_LISTO, label: "PEDIMENTO LISTO", color: "bg-purple-400", group: 5 },
      { id: MainStatus.PEDIMENTO_ENTREGADO, label: "PEDIMENTO ENTREGADO", color: "bg-slate-500", group: 5 },
      { id: MainStatus.COMBINADA, label: "COMBINADA", color: "bg-blue-500", group: 6 },
      { id: MainStatus.TRAFICO, label: "TRAFICO", color: "bg-orange-400", group: 6 },
    ];

    const INITIAL_CLIENTS = [
        "AEARO", "AUTOLIV", "AVERY", "BACA", "BAJA TACKLE", "BII DE MEXICO", "CENTRAL COAST", "DGS", "ELEVADORES",
        "DECORACIÓN", "EQUIPO INDUSTRIALES", "FABRICACIÓN", "FEMSA", "GST", "HAEMONETICS", "HUDSON", "HEATTRACK", "ICUMEDICAL",
        "INDUSTRIAS", "JAE", "MEDIMEXICO", "MEXHON", "PACIFIC PULP", "PRIME", "RICOH", "SANTEK", "SMITHS", "SMK", "SULGAN",
        "SOURCING", "SURFBLANKS", "TELEFLEX", "WESFALL", "ZIRCON", "MAXON"
    ];

    const INITIAL_RECORDS = [
      {
        id: "1",
        estatus: MainStatus.PEDIMENTO_LISTO,
        tipoOperacion: OperationType.IMPORTACION,
        noFacturasMain: "IMPIN24-04520",
        correo: "IMPORTACION IMPIN24-04520 // SIN REVISION // LORAS",
        facturas: [{ id: "f1", name: "Factura04520.pdf", type: 'FACTURA', content: "Contenido de Factura04520.pdf"}],
        txtFiles: [{ id: "t1", name: "Data04520.txt", type: 'TXT', content: "Contenido de Data04520.txt" }],
        cpFile: undefined,
        xmlFile: undefined,
        anexos: [],
        cliente: INITIAL_CLIENTS[0] || "AEARO",
        datosTxt: "Datos procesados del TXT para Cliente Alfa"
      },
      {
        id: "2",
        estatus: MainStatus.GLOSA,
        tipoOperacion: OperationType.EXPORTACION,
        noFacturasMain: "EXPEX24-0015",
        correo: "EXPORTACION EXPEX24-0015 // REVISION PENDIENTE // XYZ",
        facturas: [
          { id: "f2-1", name: "FacturaExport001.pdf", type: 'FACTURA', content: "Contenido FEX001" },
          { id: "f2-2", name: "PackingList001.pdf", type: 'FACTURA', content: "Contenido PL001" },
        ],
        txtFiles: [{ id: "t2", name: "ExportData001.txt", type: 'TXT', content: "Contenido ExportData001" }],
        cpFile: { id: "cp2", name: "CartaPorte001.xml", type: 'CP', content: "Contenido CP001" },
        xmlFile: { id: "x2", name: "Pedimento001.xml", type: 'XML', content: "Contenido Pedimento001" },
        anexos: [{id: "a2-1", name: "CertificadoOrigen.pdf", type: "ANEXO", content: "Contenido CO"}],
        cliente: INITIAL_CLIENTS[1] || "AUTOLIV",
        datosTxt: "Datos procesados del TXT para Cliente Beta"
      }
    ];

    const MAX_INVOICES = 10;
    const MAX_ANNEXES = 15;
    const MAX_TXT_FILES = 10;
    
    const SOAA_RECORDS_KEY = 'SOAA_RECORDS_V2';
    const SOAA_CLIENTS_KEY = 'SOAA_CLIENTS_V2';


    // --- Content from components/InlineAddFileButton.tsx ---
    const InlineAddFileButton = ({ recordId, fileType, onFileSelected, accept, buttonText, className }) => {
        const fileInputRef = React.useRef(null);

        const handleTriggerFileInput = () => {
            fileInputRef.current?.click();
        };

        const handleFileChange = (event) => {
            if (event.target.files && event.target.files[0]) {
                onFileSelected(event.target.files[0]);
                event.target.value = ''; 
            }
        };

        const defaultButtonText = `Añadir ${fileType}`;

        return React.createElement(React.Fragment, null,
            React.createElement("input", {
                type: "file",
                ref: fileInputRef,
                onChange: handleFileChange,
                className: "hidden",
                accept: accept || '*/*'
            }),
            React.createElement("button", {
                type: "button",
                onClick: handleTriggerFileInput,
                className: `text-xs bg-teal-500 hover:bg-teal-600 text-white py-1 px-2 rounded transition-colors duration-150 ${className || ''}`,
                title: buttonText || defaultButtonText
            },
                React.createElement("i", { className: "fas fa-plus mr-1" }),
                buttonText || defaultButtonText
            )
        );
    };

    // --- Content from components/FileChip.tsx ---
    const FileChip = ({ file, recordId, onDeleteFile }) => {
        const handleDownload = () => {
            if (file.fileObject) {
                const url = URL.createObjectURL(file.fileObject);
                const a = document.createElement('a');
                a.href = url;
                a.download = file.name;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
            } else {
                const content = file.content || `Contenido simulado para ${file.name}`;
                const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = file.name;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
            }
        };

        const displayName = file.name.length > 25 ? `${file.name.substring(0, 22)}...` : file.name;

        return React.createElement("div", { className: "bg-slate-200 text-slate-700 px-2 py-1 rounded-md text-xs inline-flex items-center mr-1 mb-1 flex-wrap" },
            React.createElement("i", { className: "fas fa-file-alt mr-1" }),
            React.createElement("span", { className: "mr-1", title: file.name }, displayName),
            React.createElement("button", {
                onClick: handleDownload,
                className: "ml-2 text-blue-500 hover:text-blue-700",
                title: `Descargar ${file.name}`,
                "aria-label": `Descargar ${file.name}`
            }, React.createElement("i", { className: "fas fa-download" })),
            React.createElement("button", {
                onClick: () => onDeleteFile(recordId, file.id, file.type),
                className: "ml-1.5 text-red-500 hover:text-red-700",
                title: `Eliminar ${file.name}`,
                "aria-label": `Eliminar ${file.name}`
            }, React.createElement("i", { className: "fas fa-times-circle" }))
        );
    };

    // --- Content from components/StatusSidebar.tsx ---
    const StatusSidebar = ({ statusConfigs, selectedStatus, onSelectStatus }) => {
        const groups = Array.from(new Set(statusConfigs.map(s => s.group))).sort((a,b) => a-b);

        return React.createElement("aside", { className: "w-64 bg-slate-700 p-3 space-y-2 overflow-y-auto text-sm" },
            React.createElement("button", {
                onClick: () => onSelectStatus(null),
                className: `w-full text-left px-3 py-2 rounded-md font-medium transition-colors duration-150 ${selectedStatus === null ? 'bg-blue-500 text-white' : 'bg-slate-600 text-slate-200 hover:bg-slate-500'}`
            }, "TODOS LOS ESTATUS"),
            groups.map(group => (
                React.createElement("div", { key: `group-${group}`, className: "space-y-1" },
                    statusConfigs.filter(s => s.group === group).map((status) => (
                        React.createElement("button", {
                            key: status.id,
                            onClick: () => onSelectStatus(status.id),
                            className: `w-full text-left px-3 py-2 rounded-md font-medium transition-colors duration-150 ${status.textColor ? status.textColor : 'text-white'} ${selectedStatus === status.id ? 'ring-2 ring-offset-2 ring-offset-slate-700 ring-white ' + status.color : status.color} hover:opacity-80`
                        }, status.label)
                    ))
                )
            ))
        );
    };
    
    // --- Content from components/ClientTabs.tsx (with delete functionality) ---
    const ClientTabs = ({ clients, selectedClient, onSelectClient, newClientName, onSetNewClientName, onAddClient, onDeleteClient, clientError }) => {
        const handleAddClientKeyDown = (e) => {
            if (e.key === 'Enter') {
                onAddClient();
            }
        };
        
        const handleDelete = (e, clientName) => {
            e.stopPropagation(); 
            onDeleteClient(clientName);
        };

        return React.createElement("div", { className: "bg-slate-700 p-2 shadow-md print:hidden" },
            React.createElement("div", { className: "flex items-center space-x-2 overflow-x-auto pb-2" },
                React.createElement("button", {
                    onClick: () => onSelectClient(null),
                    className: `px-4 py-2 text-sm font-medium rounded-md transition-colors duration-150 whitespace-nowrap ${selectedClient === null ? 'bg-blue-500 text-white' : 'bg-slate-600 text-slate-200 hover:bg-slate-500'}`,
                    "aria-pressed": selectedClient === null
                }, "Todos los Clientes"),
                clients.map((client) => (
                    React.createElement("div", { key: client, className: "relative group flex items-center" },
                        React.createElement("button", {
                            onClick: () => onSelectClient(client),
                            className: `pl-4 pr-8 py-2 text-sm font-medium rounded-md transition-colors duration-150 whitespace-nowrap ${selectedClient === client ? 'bg-blue-500 text-white' : 'bg-slate-600 text-slate-200 hover:bg-slate-500'}`,
                            "aria-pressed": selectedClient === client
                        }, client),
                        React.createElement("button", {
                            onClick: (e) => handleDelete(e, client),
                            title: `Eliminar cliente ${client}`,
                            "aria-label": `Eliminar cliente ${client}`,
                            className: `absolute right-1 top-1/2 -translate-y-1/2 p-0.5 rounded-full text-slate-300 hover:text-red-400 hover:bg-slate-500 opacity-60 group-hover:opacity-100 transition-opacity duration-150 ${selectedClient === client ? 'text-white hover:text-red-300' : ''}`
                        }, React.createElement("i", { className: "fas fa-times fa-xs" }))
                    )
                ))
            ),
            React.createElement("div", { className: "mt-2 border-t border-slate-600 pt-2" },
                React.createElement("div", { className: "flex items-center space-x-2" },
                    React.createElement("input", {
                        type: "text",
                        value: newClientName,
                        onChange: (e) => onSetNewClientName(e.target.value),
                        onKeyDown: handleAddClientKeyDown,
                        placeholder: "Nuevo nombre de cliente",
                        className: "px-3 py-1.5 text-sm bg-slate-500 text-white placeholder-slate-300 border border-slate-400 rounded-md focus:ring-1 focus:ring-blue-400 focus:border-blue-400 outline-none flex-grow",
                        "aria-label": "Nombre del nuevo cliente",
                        "aria-invalid": !!clientError,
                        "aria-describedby": clientError ? "client-error-message" : undefined
                    }),
                    React.createElement("button", {
                        onClick: onAddClient,
                        className: "bg-green-500 hover:bg-green-600 text-white font-semibold px-4 py-1.5 rounded-md text-sm transition-colors duration-150",
                        "aria-label": "Añadir nuevo cliente"
                    }, 
                        React.createElement("i", { className: "fas fa-plus mr-1 sm:mr-2" }),
                        React.createElement("span", { className: "hidden sm:inline" }, "Añadir Cliente")
                    )
                ),
                clientError && (
                    React.createElement("p", { id: "client-error-message", className: "text-red-400 text-xs mt-1 ml-1", role: "alert" }, clientError)
                )
            )
        );
    };

    // --- Content from components/RecordTable.tsx (with Cliente column and updated widths) ---
    const SortableHeader = ({ title, onSort, className }) => {
        return React.createElement("th", { className: `px-3 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider sticky top-0 bg-slate-600 ${className || ''}` },
            React.createElement("div", { className: "flex items-center" },
                title,
                onSort && React.createElement("i", { className: "fas fa-sort ml-2 text-slate-400" })
            )
        );
    };

    const RecordTable = ({ records, onEditRecord, onDeleteRecord, onDeleteFile, onUpdateRecordStatus, onAddSpecificFile, onAddFileToArray, maxInvoices, maxTxtFiles, maxAnnexes }) => {
        const [expandedAnnexes, setExpandedAnnexes] = useState({});

        const toggleAnnexes = (recordId) => {
            setExpandedAnnexes(prev => ({ ...prev, [recordId]: !prev[recordId] }));
        };
        
        const getStatusStyling = (status) => {
            const config = STATUS_CONFIG.find(s => s.id === status);
            return {
                bgColor: config ? config.color : 'bg-gray-500',
                textColor: config ? (config.textColor || 'text-white') : 'text-white',
            };
        };

        if (!records || records.length === 0) {
            return React.createElement("div", { className: "text-center text-gray-500 py-10" }, "No hay registros para mostrar. Seleccione un estado o añada un nuevo registro.");
        }

        return React.createElement("div", { className: "shadow-lg rounded-lg overflow-x-auto" },
            React.createElement("table", { className: "min-w-full divide-y divide-gray-300" },
                React.createElement("thead", { className: "bg-slate-600" },
                    React.createElement("tr", null,
                        React.createElement(SortableHeader, { title: "Estatus", className: "w-[50%]" }),
                        React.createElement(SortableHeader, { title: "Cliente", className: "w-[6%]" }),
                        React.createElement(SortableHeader, { title: "Tipo Op.", className: "w-[6%]" }),
                        React.createElement(SortableHeader, { title: "No. Factura(s)", className: "w-[6%]" }),
                        React.createElement(SortableHeader, { title: "Correo / Descripción", className: "w-[4%]" }),
                        React.createElement(SortableHeader, { title: "Facturas", className: "w-[4%]" }),
                        React.createElement(SortableHeader, { title: "TXT", className: "w-[4%]" }),
                        React.createElement(SortableHeader, { title: "CP", className: "w-[5%]" }),
                        React.createElement(SortableHeader, { title: "XML", className: "w-[5%]" }),
                        React.createElement(SortableHeader, { title: "Anexos", className: "w-[4%]" }),
                        React.createElement("th", { className: "px-3 py-3 text-left text-xs font-semibold text-white uppercase tracking-wider sticky top-0 bg-slate-600 w-[4%]" }, "Acciones")
                    )
                ),
                React.createElement("tbody", { className: "bg-white divide-y divide-gray-200" },
                    records.map((record) => {
                        const statusStyles = getStatusStyling(record.estatus);
                        return React.createElement("tr", { key: record.id, className: "hover:bg-gray-50 transition-colors duration-150 align-top" },
                            React.createElement("td", { className: `p-0 align-middle ${statusStyles.bgColor}` },
                                React.createElement("select", {
                                    value: record.estatus,
                                    onChange: (e) => onUpdateRecordStatus(record.id, e.target.value),
                                    onClick: (e) => e.stopPropagation(),
                                    className: `w-full h-full py-2 px-0.5 border-none outline-none appearance-none text-left text-xs font-medium cursor-pointer bg-transparent ${statusStyles.textColor} status-select`,
                                    "aria-label": `Cambiar estado principal para ${record.noFacturasMain || 'registro'}`
                                },
                                    STATUS_CONFIG.map(statusConf => (
                                        React.createElement("option", { key: statusConf.id, value: statusConf.id }, statusConf.label)
                                    ))
                                )
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700 whitespace-nowrap" }, record.cliente || 'N/A'),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700 whitespace-nowrap" }, record.tipoOperacion),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700 whitespace-nowrap break-words" }, record.noFacturasMain),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700 whitespace-normal break-words max-w-xs" }, record.correo),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700" },
                                (record.facturas || []).map(file => React.createElement(FileChip, { key: file.id, file: file, recordId: record.id, onDeleteFile: onDeleteFile })),
                                (record.facturas || []).length < maxInvoices && React.createElement(InlineAddFileButton, { recordId: record.id, fileType: "FACTURA", onFileSelected: (file) => onAddFileToArray(record.id, "FACTURA", file), accept: ".pdf,.jpg,.jpeg,.png" })
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700" },
                                (record.txtFiles || []).map(file => React.createElement(FileChip, { key: file.id, file: file, recordId: record.id, onDeleteFile: onDeleteFile })),
                                (record.txtFiles || []).length < maxTxtFiles && React.createElement(InlineAddFileButton, { recordId: record.id, fileType: "TXT", onFileSelected: (file) => onAddFileToArray(record.id, "TXT", file), accept: ".txt" })
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700" },
                                record.cpFile ? React.createElement(FileChip, { file: record.cpFile, recordId: record.id, onDeleteFile: onDeleteFile }) : React.createElement(InlineAddFileButton, { recordId: record.id, fileType: "CP", onFileSelected: (file) => onAddSpecificFile(record.id, "CP", file), accept: ".xml,.pdf" })
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700" },
                                record.xmlFile ? React.createElement(FileChip, { file: record.xmlFile, recordId: record.id, onDeleteFile: onDeleteFile }) : React.createElement(InlineAddFileButton, { recordId: record.id, fileType: "XML", onFileSelected: (file) => onAddSpecificFile(record.id, "XML", file), accept: ".xml" })
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700" },
                                (record.anexos || []).slice(0,1).map(file => React.createElement(FileChip, { key: file.id, file: file, recordId: record.id, onDeleteFile: onDeleteFile })),
                                (record.anexos || []).length > 1 && React.createElement("button", { onClick: () => toggleAnnexes(record.id), className: "text-blue-500 text-xs ml-1 hover:underline" }, `(${(record.anexos || []).length - 1} más)`),
                                expandedAnnexes[record.id] && (record.anexos || []).slice(1).map(file => React.createElement("div", { key: file.id, className: "mt-1" }, React.createElement(FileChip, { file: file, recordId: record.id, onDeleteFile: onDeleteFile }))),
                                (record.anexos || []).length < maxAnnexes && React.createElement("div", { className: (record.anexos || []).length > 0 ? "mt-1" : ""}, React.createElement(InlineAddFileButton, { recordId: record.id, fileType: "ANEXO", onFileSelected: (file) => onAddFileToArray(record.id, "ANEXO", file), accept: ".pdf,.doc,.docx,.xls,.xlsx,.jpg,.jpeg,.png,.txt,.xml" }))
                            ),
                            React.createElement("td", { className: "px-2 py-2 text-xs text-gray-700 whitespace-nowrap" },
                                React.createElement("div", { className: "flex flex-col items-center space-y-1" },
                                    React.createElement("button", { onClick: () => onEditRecord(record), className: "text-blue-600 hover:text-blue-800", title: "Editar", "aria-label": "Editar registro" }, React.createElement("i", { className: "fas fa-edit fa-fw" })),
                                    React.createElement("button", { onClick: () => onDeleteRecord(record.id), className: "text-red-600 hover:text-red-800", title: "Eliminar", "aria-label": "Eliminar registro" }, React.createElement("i", { className: "fas fa-trash-alt fa-fw" }))
                                )
                            )
                        );
                    })
                )
            )
        );
    };

    // --- Content from components/RecordFormModal.tsx ---
    const FileInputSection = ({ label, files, onFileSelected, onRemoveFile, fileType, maxFiles, singleFile, accept }) => {
        const fileInputRef = React.useRef(null);

        const handleTriggerFileInput = () => {
            fileInputRef.current?.click();
        };

        const handleFileChange = (event) => {
            if (event.target.files && event.target.files[0]) {
                onFileSelected(event.target.files[0]);
                event.target.value = ''; 
            }
        };

        const canAddMore = singleFile ? files.length === 0 : (maxFiles !== undefined ? files.length < maxFiles : true);

        return React.createElement("div", { className: "mb-4 p-3 border border-gray-300 rounded-md bg-gray-50" },
            React.createElement("input", { type: "file", ref: fileInputRef, onChange: handleFileChange, className: "hidden", accept: accept || '*/*' }),
            React.createElement("label", { className: "block text-sm font-medium text-gray-700 mb-1" }, `${label} ${maxFiles !== undefined ? `(Máx: ${maxFiles})` : ''}`),
            files.map((file) => (
                React.createElement("div", { key: file.id, className: "flex items-center justify-between bg-gray-100 p-1.5 rounded text-xs mb-1" },
                    React.createElement("span", null, React.createElement("i", { className: "fas fa-file-alt mr-2 text-gray-500" }), file.name),
                    React.createElement("button", { type: "button", onClick: () => onRemoveFile(file.id, file.type), className: "text-red-500 hover:text-red-700" }, React.createElement("i", { className: "fas fa-trash" }))
                )
            )),
            canAddMore && React.createElement("button", { type: "button", onClick: handleTriggerFileInput, className: "mt-1 text-xs bg-blue-500 hover:bg-blue-600 text-white py-1 px-2 rounded" }, React.createElement("i", { className: "fas fa-plus mr-1" }), `Añadir ${singleFile ? '' : 'otro '}archivo`),
            !canAddMore && files.length > 0 && React.createElement("p", { className: "text-xs text-gray-500 mt-1" }, singleFile ? 'Archivo adjuntado.' : 'Máximo de archivos alcanzado.')
        );
    };

    const RecordFormModal = ({ isOpen, onClose, onSave, existingRecord, statusOptions, operationTypes, maxInvoices, maxAnnexes, maxTxtFiles, currentClient, allClients }) => {
        const [record, setRecord] = useState({});

        useEffect(() => {
            if (existingRecord) {
                setRecord(existingRecord);
            } else {
                setRecord({
                    estatus: statusOptions[0]?.id || MainStatus.REVISION,
                    tipoOperacion: operationTypes[0] || OperationType.IMPORTACION,
                    noFacturasMain: '',
                    correo: '',
                    cliente: currentClient || '', 
                    datosTxt: '',
                    facturas: [],
                    txtFiles: [], 
                    cpFile: undefined,
                    xmlFile: undefined,
                    anexos: [],
                });
            }
        }, [existingRecord, isOpen, statusOptions, operationTypes, currentClient]);

        const handleChange = (e) => {
            const { name, value } = e.target;
            setRecord(prev => ({ ...prev, [name]: value }));
        };

        const addFileToState = useCallback((selectedFile, fileType) => {
            const newFile = { 
                id: `${fileType}-${Date.now()}-${selectedFile.name}`,
                name: selectedFile.name,
                type: fileType,
                fileObject: selectedFile 
            };

            setRecord(prev => {
                const currentRecordState = { ...prev };
                if (fileType === 'FACTURA') {
                    currentRecordState.facturas = [...(currentRecordState.facturas || []), newFile].slice(0, maxInvoices);
                } else if (fileType === 'TXT') {
                    currentRecordState.txtFiles = [...(currentRecordState.txtFiles || []), newFile].slice(0, maxTxtFiles);
                } else if (fileType === 'CP') {
                    currentRecordState.cpFile = newFile;
                } else if (fileType === 'XML') {
                    currentRecordState.xmlFile = newFile;
                } else if (fileType === 'ANEXO') {
                    currentRecordState.anexos = [...(currentRecordState.anexos || []), newFile].slice(0, maxAnnexes);
                }
                return currentRecordState;
            });
        }, [maxInvoices, maxAnnexes, maxTxtFiles]);

        const handleRemoveFile = useCallback((fileId, fileType) => {
            setRecord(prev => {
                const currentRecordState = { ...prev };
                if (fileType === 'FACTURA') {
                    currentRecordState.facturas = (currentRecordState.facturas || []).filter(f => f.id !== fileId);
                } else if (fileType === 'TXT') {
                    currentRecordState.txtFiles = (currentRecordState.txtFiles || []).filter(f => f.id !== fileId);
                } else if (fileType === 'CP' && currentRecordState.cpFile?.id === fileId) {
                    currentRecordState.cpFile = undefined;
                } else if (fileType === 'XML' && currentRecordState.xmlFile?.id === fileId) {
                    currentRecordState.xmlFile = undefined;
                } else if (fileType === 'ANEXO') {
                    currentRecordState.anexos = (currentRecordState.anexos || []).filter(f => f.id !== fileId);
                }
                return currentRecordState;
            });
        }, []);

        const handleSubmit = (e) => {
            e.preventDefault();
            if (!record.noFacturasMain || !record.correo) {
                alert("Por favor, complete los campos obligatorios: No. Factura(s) Principal y Correo/Descripción.");
                return;
            }
            onSave(record);
        };

        if (!isOpen) return null;

        return React.createElement("div", { className: "fixed inset-0 bg-black bg-opacity-50 backdrop-blur-sm flex items-center justify-center p-4 z-50 print:hidden" },
            React.createElement("div", { className: "bg-white p-6 rounded-lg shadow-xl w-full max-w-2xl max-h-[90vh] flex flex-col" },
                React.createElement("div", { className: "flex justify-between items-center mb-4" },
                    React.createElement("h2", { className: "text-xl font-semibold text-gray-800" }, existingRecord ? 'Editar Registro' : 'Nuevo Registro'),
                    React.createElement("button", { onClick: onClose, className: "text-gray-500 hover:text-gray-700" }, React.createElement("i", { className: "fas fa-times fa-lg" }))
                ),
                React.createElement("form", { onSubmit: handleSubmit, className: "overflow-y-auto pr-2 flex-grow custom-scrollbar" },
                    React.createElement("div", { className: "grid grid-cols-1 md:grid-cols-2 gap-4 mb-4" },
                        React.createElement("div", null,
                            React.createElement("label", { htmlFor: "estatus", className: "block text-sm font-medium text-gray-700" }, "Estatus"),
                            React.createElement("select", { id: "estatus", name: "estatus", value: record.estatus, onChange: handleChange, className: "mt-1 block w-full py-2 px-3 border border-gray-300 bg-white rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" },
                                statusOptions.map(opt => React.createElement("option", { key: opt.id, value: opt.id }, opt.label))
                            )
                        ),
                        React.createElement("div", null,
                            React.createElement("label", { htmlFor: "tipoOperacion", className: "block text-sm font-medium text-gray-700" }, "Tipo de Operación"),
                            React.createElement("select", { id: "tipoOperacion", name: "tipoOperacion", value: record.tipoOperacion, onChange: handleChange, className: "mt-1 block w-full py-2 px-3 border border-gray-300 bg-white rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" },
                                operationTypes.map(opt => React.createElement("option", { key: opt, value: opt }, opt))
                            )
                        )
                    ),
                    React.createElement("div", { className: "mb-4" },
                        React.createElement("label", { htmlFor: "noFacturasMain", className: "block text-sm font-medium text-gray-700" }, "No. Factura(s) Principal *"),
                        React.createElement("input", { type: "text", id: "noFacturasMain", name: "noFacturasMain", value: record.noFacturasMain || '', onChange: handleChange, required: true, className: "mt-1 block w-full py-2 px-3 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" })
                    ),
                    React.createElement("div", { className: "mb-4" },
                        React.createElement("label", { htmlFor: "correo", className: "block text-sm font-medium text-gray-700" }, "Correo / Descripción *"),
                        React.createElement("textarea", { id: "correo", name: "correo", rows: 2, value: record.correo || '', onChange: handleChange, required: true, className: "mt-1 block w-full py-2 px-3 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" })
                    ),
                    React.createElement("div", { className: "mb-4" },
                        React.createElement("label", { htmlFor: "cliente", className: "block text-sm font-medium text-gray-700" }, "Cliente"),
                        (!existingRecord && currentClient === null) ? 
                            React.createElement("select", { id: "cliente", name: "cliente", value: record.cliente || '', onChange: handleChange, className: "mt-1 block w-full py-2 px-3 border border-gray-300 bg-white rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" },
                                React.createElement("option", { value: "" }, "Seleccione un cliente o déjelo vacío"),
                                allClients.map(clientName => React.createElement("option", { key: clientName, value: clientName }, clientName))
                            ) :
                            React.createElement("input", { type: "text", id: "cliente", name: "cliente", value: record.cliente || '', onChange: handleChange, readOnly: (!existingRecord && !!currentClient) || !!existingRecord, className: `mt-1 block w-full py-2 px-3 border border-gray-300 rounded-md shadow-sm sm:text-sm ${((!existingRecord && !!currentClient) || !!existingRecord) ? 'bg-gray-100' : 'bg-white focus:outline-none focus:ring-indigo-500 focus:border-indigo-500'}` }),
                        ((!existingRecord && !!currentClient) || !!existingRecord) && React.createElement("p", { className: "text-xs text-gray-500 mt-0.5" }, "El cliente se asigna según la pestaña activa o el registro existente.")
                    ),
                    React.createElement("div", { className: "mb-4" },
                        React.createElement("label", { htmlFor: "datosTxt", className: "block text-sm font-medium text-gray-700" }, "Datos de TXT (Opcional)"),
                        React.createElement("textarea", { id: "datosTxt", name: "datosTxt", rows: 2, value: record.datosTxt || '', onChange: handleChange, className: "mt-1 block w-full py-2 px-3 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm" })
                    ),
                    React.createElement(FileInputSection, { label: "Facturas Adjuntas", files: record.facturas || [], onFileSelected: (file) => addFileToState(file, 'FACTURA'), onRemoveFile: handleRemoveFile, fileType: "FACTURA", maxFiles: maxInvoices, accept: ".pdf,.jpg,.jpeg,.png" }),
                    React.createElement("div", { className: "grid grid-cols-1 md:grid-cols-3 gap-4" },
                        React.createElement(FileInputSection, { label: "Archivos TXT", files: record.txtFiles || [], onFileSelected: (file) => addFileToState(file, 'TXT'), onRemoveFile: handleRemoveFile, fileType: "TXT", maxFiles: maxTxtFiles, accept: ".txt" }),
                        React.createElement(FileInputSection, { label: "Archivo CP", files: record.cpFile ? [record.cpFile] : [], onFileSelected: (file) => addFileToState(file, 'CP'), onRemoveFile: handleRemoveFile, fileType: "CP", singleFile: true, accept: ".xml,.pdf" }),
                        React.createElement(FileInputSection, { label: "Archivo XML", files: record.xmlFile ? [record.xmlFile] : [], onFileSelected: (file) => addFileToState(file, 'XML'), onRemoveFile: handleRemoveFile, fileType: "XML", singleFile: true, accept: ".xml" })
                    ),
                    React.createElement(FileInputSection, { label: "Anexos Adicionales", files: record.anexos || [], onFileSelected: (file) => addFileToState(file, 'ANEXO'), onRemoveFile: handleRemoveFile, fileType: "ANEXO", maxFiles: maxAnnexes, accept: ".pdf,.doc,.docx,.xls,.xlsx,.jpg,.jpeg,.png,.txt,.xml" }),
                    React.createElement("div", { className: "mt-6 flex justify-end space-x-3" },
                        React.createElement("button", { type: "button", onClick: onClose, className: "px-4 py-2 text-sm font-medium text-gray-700 bg-gray-100 rounded-md hover:bg-gray-200 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-gray-500" }, "Cancelar"),
                        React.createElement("button", { type: "submit", className: "px-4 py-2 text-sm font-medium text-white bg-blue-600 rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-blue-500" }, existingRecord ? 'Guardar Cambios' : 'Crear Registro')
                    )
                )
            )
        );
    };

    // --- Content from App.tsx (with LocalStorage sync & delete client functionality) ---
    const App = () => {
        const [records, setRecords] = useState(() => {
            try {
                const savedRecords = localStorage.getItem(SOAA_RECORDS_KEY);
                const processRecordFilesOnLoad = r => ({
                    ...r,
                    facturas: (r.facturas || []).map(f => ({...f, fileObject: undefined})),
                    txtFiles: (r.txtFiles || []).map(f => ({...f, fileObject: undefined})),
                    anexos: (r.anexos || []).map(f => ({...f, fileObject: undefined})),
                    cpFile: r.cpFile ? {...r.cpFile, fileObject: undefined} : undefined,
                    xmlFile: r.xmlFile ? {...r.xmlFile, fileObject: undefined} : undefined
                });
                return savedRecords ? JSON.parse(savedRecords).map(processRecordFilesOnLoad) : INITIAL_RECORDS.map(processRecordFilesOnLoad);
            } catch (error) {
                console.error("Error loading records from localStorage:", error);
                return INITIAL_RECORDS.map(r => ({...r, facturas: (r.facturas || []).map(f => ({...f, fileObject: undefined})), txtFiles: (r.txtFiles || []).map(f => ({...f, fileObject: undefined})), anexos: (r.anexos || []).map(f => ({...f, fileObject: undefined})), cpFile: r.cpFile ? {...r.cpFile, fileObject: undefined} : undefined, xmlFile: r.xmlFile ? {...r.xmlFile, fileObject: undefined} : undefined}));
            }
        });
        const [selectedStatus, setSelectedStatus] = useState(null);
        const [isModalOpen, setIsModalOpen] = useState(false);
        const [editingRecord, setEditingRecord] = useState(null);
        
        const [clients, setClients] = useState(() => {
            try {
                const savedClients = localStorage.getItem(SOAA_CLIENTS_KEY);
                return savedClients ? JSON.parse(savedClients) : INITIAL_CLIENTS;
            } catch (error) {
                console.error("Error loading clients from localStorage:", error);
                return INITIAL_CLIENTS;
            }
        });

        const [selectedClient, setSelectedClient] = useState(null); 
        const [newClientNameInput, setNewClientNameInput] = useState('');
        const [clientError, setClientError] = useState(null);

        useEffect(() => {
            try {
                const recordsToSave = records.map(r => ({
                    ...r,
                    facturas: (r.facturas || []).map(f => ({...f, fileObject: undefined})), 
                    txtFiles: (r.txtFiles || []).map(f => ({...f, fileObject: undefined})),
                    anexos: (r.anexos || []).map(f => ({...f, fileObject: undefined})),
                    cpFile: r.cpFile ? {...r.cpFile, fileObject: undefined} : undefined,
                    xmlFile: r.xmlFile ? {...r.xmlFile, fileObject: undefined} : undefined
                }));
                localStorage.setItem(SOAA_RECORDS_KEY, JSON.stringify(recordsToSave));
            } catch (error) {
                console.error("Error saving records to localStorage:", error);
            }
        }, [records]);

        useEffect(() => {
             try {
                localStorage.setItem(SOAA_CLIENTS_KEY, JSON.stringify(clients));
            } catch (error) {
                console.error("Error saving clients to localStorage:", error);
            }
        }, [clients]);

        useEffect(() => {
            const handleStorageChange = (event) => {
                try {
                    const processRecordFilesOnLoad = r => ({
                        ...r,
                        facturas: (r.facturas || []).map(f => ({...f, fileObject: undefined})),
                        txtFiles: (r.txtFiles || []).map(f => ({...f, fileObject: undefined})),
                        anexos: (r.anexos || []).map(f => ({...f, fileObject: undefined})),
                        cpFile: r.cpFile ? {...r.cpFile, fileObject: undefined} : undefined,
                        xmlFile: r.xmlFile ? {...r.xmlFile, fileObject: undefined} : undefined
                    });

                    if (event.key === SOAA_RECORDS_KEY && event.newValue) {
                        setRecords(JSON.parse(event.newValue).map(processRecordFilesOnLoad));
                    }
                    if (event.key === SOAA_CLIENTS_KEY && event.newValue) {
                        setClients(JSON.parse(event.newValue));
                    }
                } catch (error) {
                     console.error("Error processing storage event:", error);
                }
            };
            window.addEventListener('storage', handleStorageChange);
            return () => {
                window.removeEventListener('storage', handleStorageChange);
            };
        }, []);


        const handleSelectClient = (clientName) => {
            setSelectedClient(clientName);
        };

        const handleSetNewClientName = (name) => {
            setNewClientNameInput(name);
            if (clientError) setClientError(null); 
        };

        const handleAddClient = () => {
            const trimmedName = newClientNameInput.trim();
            if (trimmedName && !clients.includes(trimmedName)) {
                setClients(prevClients => [...prevClients, trimmedName]);
                setSelectedClient(trimmedName); 
                setNewClientNameInput('');
                setClientError(null);
            } else if (clients.includes(trimmedName)) {
                setClientError('El cliente ya existe.');
            } else {
                setClientError('Por favor, ingrese un nombre de cliente válido.');
            }
        };

        const handleDeleteClient = (clientNameToDelete) => {
            const clientHasRecords = records.some(record => record.cliente === clientNameToDelete);
            if (clientHasRecords) {
                alert(`El cliente "${clientNameToDelete}" tiene operaciones registradas y no puede ser eliminado. Por favor, reasigne o elimine primero sus operaciones.`);
                return;
            }

            if (window.confirm(`¿Está seguro de que desea eliminar al cliente "${clientNameToDelete}"? Esta acción no se puede deshacer.`)) {
                setClients(prevClients => prevClients.filter(client => client !== clientNameToDelete));
                if (selectedClient === clientNameToDelete) {
                    setSelectedClient(null); 
                }
            }
        };
        
        const filterRecords = useCallback(() => {
            return records.filter(record => {
                const statusMatch = selectedStatus ? record.estatus === selectedStatus : true;
                const clientMatch = selectedClient ? record.cliente === selectedClient : true;
                return statusMatch && clientMatch;
            });
        }, [records, selectedStatus, selectedClient]);

        const filteredRecords = useMemo(() => filterRecords(), [filterRecords]);

        const handleOpenModal = (recordToEdit) => {
            setEditingRecord(recordToEdit || null);
            setIsModalOpen(true);
        };

        const handleCloseModal = () => {
            setIsModalOpen(false);
            setEditingRecord(null);
        };

        const handleSaveRecord = (recordFromModal) => {
            let updatedRecords;
            if (editingRecord) {
                updatedRecords = records.map(r => r.id === recordFromModal.id ? recordFromModal : r);
            } else {
                const newRecordId = Date.now().toString();
                const recordToSave = { ...recordFromModal, id: newRecordId };
                 if (selectedClient && !recordToSave.cliente) { 
                    recordToSave.cliente = selectedClient;
                 }
                updatedRecords = [...records, recordToSave];
            }
            setRecords(updatedRecords);
            handleCloseModal();
        };

        const handleDeleteRecord = (recordId) => {
            setRecords(records.filter(r => r.id !== recordId));
        };

        const handleDeleteFileFromRecord = (recordId, fileId, fileType) => {
            setRecords(records.map(record => {
                if (record.id === recordId) {
                    const newRecord = { ...record };
                    if (fileType === 'FACTURA') newRecord.facturas = newRecord.facturas.filter(f => f.id !== fileId);
                    else if (fileType === 'TXT') newRecord.txtFiles = (newRecord.txtFiles || []).filter(f => f.id !== fileId);
                    else if (fileType === 'CP') newRecord.cpFile = undefined;
                    else if (fileType === 'XML') newRecord.xmlFile = undefined;
                    else if (fileType === 'ANEXO') newRecord.anexos = newRecord.anexos.filter(f => f.id !== fileId);
                    return newRecord;
                }
                return record;
            }));
        };

        const handleUpdateRecordStatus = (recordId, newStatus) => {
            setRecords(prevRecords => prevRecords.map(record => record.id === recordId ? { ...record, estatus: newStatus } : record));
        };

        const handleAddSpecificFileToRecord = (recordId, fileType, selectedFile) => {
            if (!selectedFile) return;
            const newAttachedFile = { id: `${fileType}-${Date.now()}-${selectedFile.name}`, name: selectedFile.name, type: fileType, fileObject: selectedFile };
            setRecords(prevRecords => prevRecords.map(record => {
                if (record.id === recordId) {
                    if (fileType === 'CP') return { ...record, cpFile: newAttachedFile };
                    if (fileType === 'XML') return { ...record, xmlFile: newAttachedFile };
                }
                return record;
            }));
        };

        const handleAddFileToArrayInRecord = (recordId, fileType, selectedFile) => {
            if (!selectedFile) return;
            const newAttachedFile = { id: `${fileType}-${Date.now()}-${selectedFile.name}`, name: selectedFile.name, type: fileType, fileObject: selectedFile };
            setRecords(prevRecords => prevRecords.map(record => {
                if (record.id === recordId) {
                    const updatedRecord = { ...record };
                    if (fileType === 'FACTURA') updatedRecord.facturas = [...(updatedRecord.facturas || []), newAttachedFile].slice(0, MAX_INVOICES);
                    else if (fileType === 'TXT') updatedRecord.txtFiles = [...(updatedRecord.txtFiles || []), newAttachedFile].slice(0, MAX_TXT_FILES);
                    else if (fileType === 'ANEXO') updatedRecord.anexos = [...(updatedRecord.anexos || []), newAttachedFile].slice(0, MAX_ANNEXES);
                    return updatedRecord;
                }
                return record;
            }));
        };

        return React.createElement("div", { className: "flex flex-col h-screen overflow-hidden" },
            React.createElement("header", { className: "bg-slate-800 text-white p-3 flex justify-between items-center shadow-md print:hidden" },
                React.createElement("h1", { className: "text-3xl font-bold tracking-wider" }, "SOAA"),
                React.createElement("button", { onClick: () => handleOpenModal(), className: "bg-white text-slate-800 font-semibold px-6 py-2 rounded-md hover:bg-slate-200 transition duration-150 shadow" }, "REGISTRAR")
            ),
            React.createElement(ClientTabs, { 
                clients: clients, 
                selectedClient: selectedClient, 
                onSelectClient: handleSelectClient, 
                newClientName: newClientNameInput, 
                onSetNewClientName: handleSetNewClientName, 
                onAddClient: handleAddClient, 
                onDeleteClient: handleDeleteClient, 
                clientError: clientError 
            }),
            React.createElement("div", { className: "flex flex-1 overflow-hidden" },
                React.createElement(StatusSidebar, { statusConfigs: STATUS_CONFIG, selectedStatus: selectedStatus, onSelectStatus: setSelectedStatus }),
                React.createElement("main", { className: "flex-1 p-4 overflow-auto bg-gray-200" },
                    React.createElement(RecordTable, { 
                        records: filteredRecords, 
                        onEditRecord: handleOpenModal, 
                        onDeleteRecord: handleDeleteRecord,
                        onDeleteFile: handleDeleteFileFromRecord,
                        onUpdateRecordStatus: handleUpdateRecordStatus,
                        onAddSpecificFile: handleAddSpecificFileToRecord,
                        onAddFileToArray: handleAddFileToArrayInRecord,
                        maxInvoices: MAX_INVOICES,
                        maxTxtFiles: MAX_TXT_FILES,
                        maxAnnexes: MAX_ANNEXES
                    })
                )
            ),
            isModalOpen && React.createElement(RecordFormModal, {
                isOpen: isModalOpen,
                onClose: handleCloseModal,
                onSave: handleSaveRecord,
                existingRecord: editingRecord,
                statusOptions: STATUS_CONFIG,
                operationTypes: Object.values(OperationType),
                maxInvoices: MAX_INVOICES,
                maxAnnexes: MAX_ANNEXES,
                maxTxtFiles: MAX_TXT_FILES,
                currentClient: selectedClient, 
                allClients: clients 
            })
        );
    };

    // --- Content from index.tsx (mounting logic) ---
    const rootElement = document.getElementById('root');
    if (!rootElement) {
      throw new Error("Could not find root element to mount to");
    }

    const root = ReactDOM.createRoot(rootElement);
    root.render(
      React.createElement(React.StrictMode, null,
        React.createElement(App, null)
      )
    );
  </script>
</body>
</html>
