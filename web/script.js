let dashboardData = null;
let updateInterval = null;
let chartInstance = null;

document.addEventListener('DOMContentLoaded', () => {
    console.log('Dashboard UNITEL inicializado');
    loadDashboardData();
    startAutoRefresh(5000); 
    setupEventListeners();
});

function setupEventListeners() {
    document.getElementById('btn-contratar').addEventListener('click', () => {
        iniciarContratacao();
    });
    
    document.getElementById('btn-demitir').addEventListener('click', () => {
        demitirFuncionario();
    });
    
    document.getElementById('btn-atualizar').addEventListener('click', () => {
        loadDashboardData(true);
    });
}

function startAutoRefresh(interval) {
    if (updateInterval) clearInterval(updateInterval);
    updateInterval = setInterval(() => {
        loadDashboardData(false);
    }, interval);
}

async function loadDashboardData(showLoading = true) {
    try {
        if (showLoading) showLoadingStates();
        
        const response = await fetch('/api/dashboard');
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        
        const data = await response.json();
        dashboardData = data;
        
        updateDashboardUI(data);
        updateTimestamp(data.timestamp);
        
    } catch (error) {
        console.error('Erro ao carregar dados:', error);
        showError('Falha ao carregar dados do servidor');
    }
}

function updateDashboardUI(data) {
    updateEstoque(data.estoque);
    updateRH(data.rh);
    updateVendas(data.vendas);
    updateFila(data.fila);
    updateVendasChart(data.vendas);
    updateCartoesList(data.estoque);
    updateAgencias(data.agencias);
}

function updateEstoque(estoque) {
    document.getElementById('estoque-disponivel').textContent = estoque.disponiveis;
    document.getElementById('estoque-total').textContent = estoque.total;
    
    const progress = (estoque.vendidos / estoque.total) * 100;
    document.getElementById('progress-estoque').style.width = `${progress}%`;
    
    const card = document.getElementById('card-estoque');
    if (estoque.disponiveis === 0) {
        card.style.borderLeft = '4px solid var(--danger)';
    } else if (estoque.disponiveis < 20) {
        card.style.borderLeft = '4px solid var(--warning)';
    } else {
        card.style.borderLeft = '1px solid var(--border)';
    }
}

function updateRH(rh) {
    document.getElementById('rh-ativos').textContent = rh.ativos;
    document.getElementById('rh-limite').textContent = rh.limite;
    
    const progress = (rh.ativos / rh.limite) * 100;
    document.getElementById('progress-rh').style.width = `${progress}%`;
}

function updateVendas(vendas) {
    document.getElementById('vendas-total').textContent = vendas.total;
    document.getElementById('vendas-empresas').textContent = vendas.empresas;
    document.getElementById('vendas-publico').textContent = vendas.publico;
}

function updateFila(fila) {
    document.getElementById('fila-tamanho').textContent = fila.tamanho;
    
    const badge = document.getElementById('fila-count-badge');
    badge.innerHTML = `<i class="fas fa-user-clock"></i> ${fila.tamanho} clientes`;
    
    const tbody = document.getElementById('fila-body');
    
    if (fila.clientes.length === 0) {
        tbody.innerHTML = '<tr><td colspan="5" class="empty-message"><i class="fas fa-users-slash"></i> Nenhum cliente na fila</td></tr>';
        return;
    }
    
    let html = '';
    fila.clientes.forEach(cliente => {
        const esperaFormatada = cliente.espera_minutos < 1 
            ? `${cliente.espera_segundos}s` 
            : `${cliente.espera_minutos.toFixed(1)}min`;
        
        const tipoClass = cliente.tipo === 'EMPRESA' ? 'tipo-empresa' : 'tipo-publico';
        
        html += `
            <tr>
                <td><strong>#${cliente.posicao}</strong></td>
                <td>Cliente ${cliente.id}</td>
                <td><span class="tipo-badge ${tipoClass}">${cliente.tipo}</span></td>
                <td><strong>${cliente.prioridade}</strong></td>
                <td>${esperaFormatada}</td>
            </tr>
        `;
    });
    
    tbody.innerHTML = html;
}

function updateVendasChart(vendas) {
    const ctx = document.getElementById('chart-turnos').getContext('2d');
    
    if (chartInstance) {
        chartInstance.destroy();
    }
    
    chartInstance = new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: ['Manhã', 'Tarde', 'Noite'],
            datasets: [{
                data: [
                    vendas.turnos.manha,
                    vendas.turnos.tarde,
                    vendas.turnos.noite
                ],
                backgroundColor: [
                    'rgba(37, 99, 235, 0.8)',
                    'rgba(5, 150, 105, 0.8)',
                    'rgba(217, 119, 6, 0.8)'
                ],
                borderWidth: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            cutout: '70%',
            plugins: {
                legend: {
                    display: false
                }
            }
        }
    });
    
    const legend = document.getElementById('legend-turnos');
    legend.innerHTML = `
        <div class="legend-item">
            <span class="legend-color" style="background: rgba(37, 99, 235, 0.8)"></span>
            <span>Manhã: ${vendas.turnos.manha}</span>
        </div>
        <div class="legend-item">
            <span class="legend-color" style="background: rgba(5, 150, 105, 0.8)"></span>
            <span>Tarde: ${vendas.turnos.tarde}</span>
        </div>
        <div class="legend-item">
            <span class="legend-color" style="background: rgba(217, 119, 6, 0.8)"></span>
            <span>Noite: ${vendas.turnos.noite}</span>
        </div>
    `;
}

function updateCartoesList(estoque) {
    const container = document.getElementById('lista-cartoes');
    
    if (estoque.cartoes.length === 0) {
        container.innerHTML = '<div class="empty-message">Nenhum cartão vendido</div>';
        return;
    }

    const ultimos = estoque.cartoes.slice(-5).reverse();
    
    let html = '';
    ultimos.forEach(cartao => {
        const data = cartao.hora_venda ? cartao.hora_venda.split(' ')[1] : '--:--:--';
        html += `
            <div class="cartao-item">
                <span class="cartao-id">Cartão #${String(cartao.id).padStart(3, '0')}</span>
                <span class="cartao-data">${data}</span>
            </div>
        `;
    });
    
    container.innerHTML = html;
}

function updateAgencias(agencias) {
    const container = document.getElementById('lista-agencias');
    
    let html = '';
    agencias.agencias.forEach(agencia => {
        const statusClass = agencia.status === 'ATIVA' ? 'status-ativo' : 'status-inativo';
        
        html += `
            <div class="agencia-item">
                <div class="agencia-info">
                    <span class="agencia-nome">${agencia.nome}</span>
                    <div class="agencia-stats">
                        <span>📊 ${agencia.vendas} vendas</span>
                        <span>👤 ${agencia.clientes_atendidos} atendidos</span>
                    </div>
                </div>
                <div class="agencia-status">
                    <span class="tipo-badge ${statusClass}">${agencia.status}</span>
                </div>
            </div>
        `;
    });
    
    container.innerHTML = html;
}

function updateTimestamp(timestamp) {
    document.getElementById('timestamp').textContent = timestamp;
}

function showLoadingStates() {
    const loadingElements = [
        'estoque-disponivel', 'rh-ativos', 'vendas-total', 'fila-tamanho'
    ];
    
    loadingElements.forEach(id => {
        const element = document.getElementById(id);
        if (element && element.textContent === '-') {
            element.textContent = '...';
        }
    });
}

function showError(message) {
    console.error(message);
}

async function iniciarContratacao() {
    try {
        const response = await fetch('/api/operacoes', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ tipo: 'contratar' })
        });
        
        if (response.ok) {
            showNotification('Processo de contratação iniciado', 'success');
            setTimeout(() => loadDashboardData(false), 1000);
        }
    } catch (error) {
        showNotification('Erro ao iniciar contratação', 'error');
    }
}

async function demitirFuncionario() {
    try {
        const response = await fetch('/api/operacoes', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ tipo: 'demitir' })
        });
        
        if (response.ok) {
            showNotification('Funcionário demitido', 'success');
            setTimeout(() => loadDashboardData(false), 1000);
        }
    } catch (error) {
        showNotification('Erro ao demitir funcionário', 'error');
    }
}

function showNotification(message, type = 'info') {
    const icon = type === 'success' ? 'fa-check-circle' : 'fa-exclamation-circle';
    
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.innerHTML = `<i class="fas ${icon}" style="margin-right: 8px;"></i> ${message}`;
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        padding: 12px 24px;
        background: ${type === 'success' ? 'linear-gradient(135deg, #059669, #047857)' : 'linear-gradient(135deg, #dc2626, #b91c1c)'};
        color: white;
        border-radius: 8px;
        box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        z-index: 9999;
        animation: slideIn 0.3s ease;
        display: flex;
        align-items: center;
        font-family: 'Inter', sans-serif;
    `;
    
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease';
        setTimeout(() => notification.remove(), 300);
    }, 3000);
}

const style = document.createElement('style');
style.textContent = `
    @keyframes slideIn {
        from {
            transform: translateX(100%);
            opacity: 0;
        }
        to {
            transform: translateX(0);
            opacity: 1;
        }
    }
    
    @keyframes slideOut {
        from {
            transform: translateX(0);
            opacity: 1;
        }
        to {
            transform: translateX(100%);
            opacity: 0;
        }
    }
`;
document.head.appendChild(style);