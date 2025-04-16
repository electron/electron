import React, { useState } from 'react';
import { Card, CardContent } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import html2canvas from 'html2canvas';

const TradingPlanWizard = () => {
  const [step, setStep] = useState(1);
  const [risk, setRisk] = useState(1);
  const [lossLimit, setLossLimit] = useState(3);
  const [profitLimit, setProfitLimit] = useState(8);
  const [rr, setRr] = useState('1:2');

  const [analysis, setAnalysis] = useState({
    Diario: { direccion: '', estructura: '', tipoMovimiento: '', zona: false, fvg: false, ob: '', observaciones: '' },
    H1: { fvg: false, ob: '', zona: false, direccion: '', estructura: '', tipoMovimiento: '', observaciones: '' },
    M5: { fvg: false, ob: '', zona: false, direccion: '', estructura: '', tipoMovimiento: '', observaciones: '' },
    M1: { fvg: false, ob: '', zona: false, direccion: '', estructura: '', tipoMovimiento: '', observaciones: '' }
  });

  const nextStep = () => setStep(step + 1);
  const prevStep = () => setStep(step - 1);

  const exportAsImage = async () => {
    const element = document.getElementById('resumen');
    if (!element) return;
    const canvas = await html2canvas(element);
    const image = canvas.toDataURL('image/jpeg', 1.0);
    const link = document.createElement('a');
    link.href = image;
    link.download = 'trading-plan.jpg';
    link.click();
  };

  const handleCheckbox = (tf, field) => {
    setAnalysis(prev => ({
      ...prev,
      [tf]: {
        ...prev[tf],
        [field]: !prev[tf][field]
      }
    }));
  };

  const handleSelect = (tf, field, value) => {
    setAnalysis(prev => ({
      ...prev,
      [tf]: {
        ...prev[tf],
        [field]: value
      }
    }));
  };

  const isSafeEntry = Object.values(analysis).every(
    item => item.direccion === analysis.Diario.direccion && item.fvg && item.ob !== '' && item.zona
  );

  const calculateViability = (data) => {
    let count = 0;
    if (data.fvg) count++;
    if (data.ob !== '') count++;
    if (data.zona) count++;
    if (data.direccion !== '') count++;
    if (data.tipoMovimiento !== '') count++;
    return (count / 5) * 100;
  };

  const getColor = (percentage) => {
    if (percentage < 50) return 'text-red-500';
    if (percentage >= 50 && percentage < 80) return 'text-yellow-500';
    return 'text-green-500';
  };

  return (
    <div className="min-h-screen flex items-center justify-center p-4">
      <Card className="w-full max-w-2xl p-4">
        <CardContent>
          {step === 1 && (
            <div className="space-y-4 text-center">
              <h2 className="text-2xl font-bold">Gestión de Riesgo</h2>
              <div className="space-y-2">
                <label className="block">Por operación (%):
                  <input type="number" value={risk} onChange={e => setRisk(e.target.value)} className="w-full border p-2 rounded mt-1" />
                </label>
                <label className="block">Límite de pérdidas:
                  <input type="number" value={lossLimit} onChange={e => setLossLimit(e.target.value)} className="w-full border p-2 rounded mt-1" />
                </label>
                <label className="block">Límite de ganancias mensual (%):
                  <input type="number" value={profitLimit} onChange={e => setProfitLimit(e.target.value)} className="w-full border p-2 rounded mt-1" />
                </label>
                <label className="block">RR mínimo:
                  <input type="text" value={rr} onChange={e => setRr(e.target.value)} className="w-full border p-2 rounded mt-1" />
                </label>
              </div>
              <Button onClick={nextStep} className="w-full">Siguiente</Button>
            </div>
          )}

          {step === 2 && (
            <div className="space-y-4">
              <h2 className="text-2xl font-bold text-center">Proceso de Análisis por Timeframe</h2>
              {Object.keys(analysis).map(tf => (
                <div key={tf} className="p-4 border rounded mb-4 space-y-2">
                  <h3 className="font-semibold text-lg text-center">{tf}</h3>
                  <div className="grid grid-cols-1 md:grid-cols-2 gap-2">
                    <label><input type="checkbox" checked={analysis[tf].fvg} onChange={() => handleCheckbox(tf, 'fvg')} /> FVG</label>
                    <label>OB:
                      <select value={analysis[tf].ob} onChange={e => handleSelect(tf, 'ob', e.target.value)} className="border p-1 rounded ml-2">
                        <option value="">Seleccione</option>
                        <option value="Original">Original</option>
                        <option value="Decisional">Decisional</option>
                      </select>
                    </label>
                    <label><input type="checkbox" checked={analysis[tf].zona} onChange={() => handleCheckbox(tf, 'zona')} /> Zona de Descuento</label>
                    <label>Dirección:
                      <select value={analysis[tf].direccion} onChange={e => handleSelect(tf, 'direccion', e.target.value)} className="border p-1 rounded ml-2">
                        <option value="">Seleccione</option>
                        <option value="Alcista">Alcista</option>
                        <option value="Bajista">Bajista</option>
                      </select>
                    </label>
                    <label>Estructura Anterior:
                      <select value={analysis[tf].estructura} onChange={e => handleSelect(tf, 'estructura', e.target.value)} className="border p-1 rounded ml-2">
                        <option value="">Seleccione</option>
                        <option value="BOS">BOS</option>
                        <option value="CHOCH">CHOCH</option>
                      </select>
                    </label>
                    <label>Tipo de Movimiento:
                      <select value={analysis[tf].tipoMovimiento} onChange={e => handleSelect(tf, 'tipoMovimiento', e.target.value)} className="border p-1 rounded ml-2">
                        <option value="">Seleccione</option>
                        <option value="Impulso">Impulso</option>
                        <option value="Retroceso">Retroceso</option>
                      </select>
                    </label>
                    <label>Observaciones:
                      <textarea value={analysis[tf].observaciones} onChange={e => handleSelect(tf, 'observaciones', e.target.value)} className="border p-1 rounded w-full mt-1" rows="2" placeholder="Escribe tus observaciones..." />
                    </label>
                  </div>
                </div>
              ))}
              <div className="flex justify-between">
                <Button onClick={prevStep}>Atrás</Button>
                <Button onClick={nextStep}>Siguiente</Button>
              </div>
            </div>
          )}

          {step === 3 && (
            <div className="space-y-4">
              <h2 className="text-2xl font-bold text-center">Resumen</h2>
              <div id="resumen" className="p-4 bg-white rounded shadow space-y-2">
                <ul className="space-y-2">
                  <li>📊 Riesgo por operación: {risk}%</li>
                  <li>❌ Límite de pérdidas: {lossLimit}</li>
                  <li>💰 Límite de ganancias mensual: {profitLimit}%</li>
                  <li>⚖️ RR mínimo: {rr}</li>
                </ul>
                <h3 className="font-semibold mt-4">Análisis por Timeframe</h3>
                {Object.entries(analysis).map(([tf, data]) => {
                  const viability = calculateViability(data);
                  return (
                    <div key={tf}>
                      <strong>{tf}</strong>: 
                      FVG: {data.fvg ? '✅' : '❌'}, 
                      OB: {data.ob || 'N/A'}, 
                      Zona: {data.zona ? '✅' : '❌'}, 
                      Dirección: {data.direccion || 'N/A'}, 
                      Estructura: {data.estructura || 'N/A'}, 
                      Tipo: {data.tipoMovimiento || 'N/A'}
                      <span className={`ml-2 font-bold ${getColor(viability)}`}>{viability.toFixed(0)}%</span>
                      {data.observaciones && (
                        <div className="mt-1 text-sm text-gray-600">📝 {data.observaciones}</div>
                      )}
                    </div>
                  );
                })}
                <p className="mt-4 text-lg font-bold text-center">
                  {isSafeEntry ? '✔️ Entrada a favor de tendencia' : '⚠️ Entrada en contra o dudosa'}
                </p>
              </div>
              <div className="flex justify-between">
                <Button onClick={prevStep}>Atrás</Button>
                <Button onClick={exportAsImage}>Guardar como JPG</Button>
              </div>
            </div>
          )}

        </CardContent>
      </Card>
    </div>
  );
};

export default TradingPlanWizard;

