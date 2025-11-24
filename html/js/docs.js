// API 文档交互功能
document.addEventListener('DOMContentLoaded', function() {
    console.log('[Docs] Initializing documentation interactions');
    
    // 获取元素
    const backToTop = document.getElementById('back-to-top');
    const readingProgress = document.getElementById('reading-progress');
    const mainContent = document.querySelector('.main-content');
    
    console.log('[Docs] Back to top button:', backToTop);
    console.log('[Docs] Reading progress:', readingProgress);
    
    // 返回顶部按钮点击事件
    if (backToTop) {
        backToTop.addEventListener('click', function() {
            if (mainContent) {
                mainContent.scrollTo({
                    top: 0,
                    behavior: 'smooth'
                });
            } else {
                window.scrollTo({
                    top: 0,
                    behavior: 'smooth'
                });
            }
            
            // 点击时添加缩放动画
            this.style.transform = 'translateY(0) scale(0.9)';
            setTimeout(() => {
                this.style.transform = 'translateY(0) scale(1)';
            }, 150);
        });
    }
    
    // 使用 requestAnimationFrame 优化进度条更新
    let progressTicking = false;
    
    function updateProgress() {
        const scrollableElement = mainContent || document.documentElement;
        const scrollTop = scrollableElement.scrollTop || window.scrollY;
        const scrollHeight = scrollableElement.scrollHeight - scrollableElement.clientHeight;
        const scrollPercentage = Math.min(100, Math.max(0, (scrollTop / scrollHeight) * 100));
        
        // 使用 transform 而不是 width 以获得更好的性能
        if (readingProgress) {
            readingProgress.style.width = scrollPercentage + '%';
        }
        
        // 显示/隐藏返回顶部按钮
        if (backToTop) {
            if (scrollTop > 300) {
                backToTop.classList.add('show');
            } else {
                backToTop.classList.remove('show');
            }
        }
        
        progressTicking = false;
    }
    
    // 使用 requestAnimationFrame 实现流畅的滚动监听
    const scrollHandler = function() {
        if (!progressTicking) {
            window.requestAnimationFrame(updateProgress);
            progressTicking = true;
        }
    };
    
    if (mainContent) {
        mainContent.addEventListener('scroll', scrollHandler, { passive: true });
    } else {
        window.addEventListener('scroll', scrollHandler, { passive: true });
    }
    
    // 初始调用
    updateProgress();
    
    // 高亮当前章节
    const sections = document.querySelectorAll('section[id]');
    const navLinks = document.querySelectorAll('.sidebar-nav a');
    
    function updateActiveSection() {
        let current = '';
        const scrollableElement = mainContent || document.documentElement;
        const scrollPos = (scrollableElement.scrollTop || window.scrollY) + 150;
        
        // 找到当前section
        sections.forEach(section => {
            const sectionTop = section.offsetTop;
            const sectionHeight = section.offsetHeight;
            
            if (scrollPos >= sectionTop && scrollPos < sectionTop + sectionHeight) {
                current = section.getAttribute('id');
            }
        });
        
        // 移除所有section的高亮
        sections.forEach(s => s.classList.remove('active-section'));
        
        // 高亮当前section
        if (current) {
            const currentSection = document.getElementById(current);
            if (currentSection) {
                currentSection.classList.add('active-section');
            }
        }
        
        // 更新侧边栏高亮
        navLinks.forEach(link => {
            const wasActive = link.classList.contains('active');
            const href = link.getAttribute('href');
            
            // 移除所有父级激活状态
            link.classList.remove('parent-active');
            
            if (href === '#' + current) {
                if (!wasActive) {
                    link.classList.add('active');
                    
                    // 找到父级链接并添加parent-active类
                    const parentLi = link.closest('ul ul')?.closest('li');
                    if (parentLi) {
                        const parentLink = parentLi.querySelector('a');
                        if (parentLink) {
                            parentLink.classList.add('parent-active');
                        }
                    }
                    
                    // 滚动到可见区域
                    link.scrollIntoView({ 
                        behavior: 'smooth', 
                        block: 'nearest',
                        inline: 'nearest'
                    });
                }
            } else {
                link.classList.remove('active');
            }
        });
    }
    
    window.addEventListener('scroll', updateActiveSection);
    updateActiveSection(); // Initial call
    
    // 平滑滚动到章节 - 只处理锚点链接
    console.log('[Docs] Setting up navigation links, count:', navLinks.length);
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            const href = this.getAttribute('href');
            console.log('[Docs] Link clicked:', href);
            
            // 只拦截锚点链接，不拦截页面跳转
            if (href && href.startsWith('#')) {
                e.preventDefault();
                
                const target = document.querySelector(href);
                console.log('[Docs] Target found:', target);
                
                if (target) {
                    // 先移除所有激活状态
                    navLinks.forEach(l => {
                        l.classList.remove('active');
                        l.classList.remove('parent-active');
                    });
                    
                    // 激活当前链接
                    this.classList.add('active');
                    
                    // 激活父级链接
                    const parentLi = this.closest('ul ul')?.closest('li');
                    if (parentLi) {
                        const parentLink = parentLi.querySelector('a');
                        if (parentLink) {
                            parentLink.classList.add('parent-active');
                        }
                    }
                    
                    // 平滑滚动到目标位置
                    target.scrollIntoView({
                        behavior: 'smooth',
                        block: 'start'
                    });
                    
                    // 更新URL
                    history.pushState(null, null, href);
                } else {
                    console.warn('[Docs] Target not found for:', href);
                }
            }
            // 如果是普通链接（如 index.html），让浏览器正常跳转
        });
    });
    
    // 处理页面加载时的锚点
    if (window.location.hash) {
        setTimeout(() => {
            const target = document.querySelector(window.location.hash);
            if (target) {
                const offsetTop = target.offsetTop - 20;
                window.scrollTo({
                    top: offsetTop,
                    behavior: 'smooth'
                });
            }
        }, 100);
    }
    
    // 添加面包屑导航
    function updateBreadcrumb() {
        const activeLink = document.querySelector('.sidebar-nav a.active');
        let breadcrumb = document.querySelector('.breadcrumb');
        
        if (!breadcrumb) {
            breadcrumb = document.createElement('nav');
            breadcrumb.className = 'breadcrumb';
            const contentWrapper = document.querySelector('.content-wrapper');
            if (contentWrapper && contentWrapper.firstChild) {
                contentWrapper.insertBefore(breadcrumb, contentWrapper.firstChild);
            }
        }
        
        if (activeLink) {
            const text = activeLink.textContent.trim();
            breadcrumb.innerHTML = `
                <a href="#overview">首页</a>
                <span class="separator">/</span>
                <span class="current">${text}</span>
            `;
            breadcrumb.style.display = 'flex';
        } else {
            breadcrumb.style.display = 'none';
        }
    }
    
    // 监听滚动更新面包屑
    window.addEventListener('scroll', updateBreadcrumb);
    updateBreadcrumb();
    
    // 优化滚动性能 - 使用 requestAnimationFrame
    let ticking = false;
    
    if (mainContent) {
        mainContent.addEventListener('scroll', function() {
            if (!ticking) {
                window.requestAnimationFrame(function() {
                    updateActiveSection();
                    ticking = false;
                });
                ticking = true;
            }
        });
    }
    
    // 鼠标悬停在文档标题时，侧边栏对应链接高亮
    const headingsWithId = document.querySelectorAll('h2[id], h3[id]');
    headingsWithId.forEach(heading => {
        heading.addEventListener('mouseenter', function() {
            const id = this.getAttribute('id');
            if (id) {
                // 找到对应的侧边栏链接
                const correspondingLink = document.querySelector(`.sidebar-nav a[href="#${id}"]`);
                if (correspondingLink) {
                    correspondingLink.classList.add('hover-highlight');
                }
            }
        });
        
        heading.addEventListener('mouseleave', function() {
            const id = this.getAttribute('id');
            if (id) {
                const correspondingLink = document.querySelector(`.sidebar-nav a[href="#${id}"]`);
                if (correspondingLink) {
                    correspondingLink.classList.remove('hover-highlight');
                }
            }
        });
    });
});
